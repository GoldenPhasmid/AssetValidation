#include "AssetValidationSubsystem.h"

#include "EditorValidatorBase.h"
#include "Misc/DataValidation.h"
#include "AssetValidationModule.h"
#include "AssetValidators/AssetValidator.h"
#include "Misc/ScopedSlowTask.h"
#include "Misc/UObjectToken.h"
#include "Settings/ProjectPackagingSettings.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

static int32 ValidatedAssetsCount = 0;
TStaticArray<int32, 3> ValidationResults{InPlace, 0};

static_assert(static_cast<uint8>(EDataValidationResult::Invalid)		== 0);
static_assert(static_cast<uint8>(EDataValidationResult::Valid)			== 1);
static_assert(static_cast<uint8>(EDataValidationResult::NotValidated)	== 2);

UAssetValidationSubsystem::UAssetValidationSubsystem()
{
	if (HasAllFlags(RF_ClassDefaultObject))
	{
		// this is a dirty hack to disable UEditorValidatorSubsystem creation and using this subsystem instead
		// I'm amazed that it actually works and doesn't break anything
		// there's no easy way in unreal to disable engine subsystems unless there's explicit support in ShouldCreateSubsystem
		UClass* Class = const_cast<UClass*>(UEditorValidatorSubsystem::StaticClass());
		Class->ClassFlags |= CLASS_Abstract;
	}
}

EDataValidationResult UAssetValidationSubsystem::IsStandaloneActorValid(AActor* Actor, TArray<FText>& ValidationErrors, TArray<FText>& ValidationWarnings, const EDataValidationUsecase InValidationUsecase) const
{
	++ValidatedAssetsCount;
	EDataValidationResult Result = EDataValidationResult::NotValidated;
	
	if (Actor != nullptr)
	{
		FDataValidationContext Context;
		Result = IsActorValid(Actor, Context);
		Context.SplitIssues(ValidationWarnings, ValidationErrors);

		if (Result != EDataValidationResult::Invalid)
		{
			for (auto& ValidatorPair: Validators)
			{
				UEditorValidatorBase* Validator = ValidatorPair.Value;
				if (CanUseValidator(Validator, InValidationUsecase) && Validator->CanValidateAsset(Actor))
				{
					Validator->ResetValidationState();
					Result &= Validator->ValidateLoadedAsset(Actor, ValidationErrors);
					
					ValidationWarnings.Append(Validator->GetAllWarnings());
					ensureMsgf(Validator->IsValidationStateSet(), TEXT("Validator %s did not include a pass or fail state."), *Validator->GetClass()->GetName());
				}
			}
		}
	}

	ValidationResults[static_cast<uint8>(Result)] += 1;
	return Result;
}

int32 UAssetValidationSubsystem::ValidateAssetsWithSettings(const TArray<FAssetData>& AssetDataList, const FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults) const
{
	FScopedSlowTask SlowTask(1.0f, LOCTEXT("ValidatingDataTask", "Validating Data..."));
	SlowTask.Visibility = ESlowTaskVisibility::ForceVisible;
	SlowTask.MakeDialogDelayed(.1f);

	// Broadcast the Editor event before we start validating. This lets other systems (such as Sequencer) restore the state
	// of the level to what is actually saved on disk before performing validation.
	if (FEditorDelegates::OnPreAssetValidation.IsBound())
	{
		FEditorDelegates::OnPreAssetValidation.Broadcast();
	}

	FMessageLog DataValidationLog("AssetCheck");

	int32 NumFilesChecked = 0;
	int32 NumValidFiles = 0;
	int32 NumInvalidFiles = 0;
	int32 NumSkippedFiles = 0;
	int32 NumFilesUnableToValidate = 0;
	int32 NumFilesWithWarnings = 0;

	const int32 NumFilesToValidate = AssetDataList.Num();

	// ASSET VALIDATION BEGIN
	auto GetAssetToken = [](const FAssetData& AssetData) -> TSharedRef<IMessageToken>
	{
		// use FUObjectToken if working with external package like WP actor
		// @todo: I'm dying inside from calling FastGetAsset and using MultiMap every time instead of just caching these two values.
		if (const UObject* Asset = AssetData.FastGetAsset(false); Asset && Asset->IsPackageExternal())
		{
			return FUObjectToken::Create(Asset);
		}

		return FAssetNameToken::Create(AssetData.PackageName.ToString());
	};
	// ASSET VALIDATION END
	
	auto AddAssetLogTokens = [&DataValidationLog](EMessageSeverity::Type Severity, const TArray<FText>& Messages, const FAssetData& AssetData)
	{
		if (Messages.IsEmpty())
		{
			return;
		}
		
		// ASSET VALIDATION BEGIN
		// use object name if working with external package
		const UObject* Asset = AssetData.FastGetAsset(false);
		const bool bExternalPackage = Asset && Asset->IsPackageExternal();
		
		const FString PackageName = bExternalPackage ? Asset->GetName() : AssetData.PackageName.ToString();
		// ASSET VALIDATION END
		
		for (const FText& Msg : Messages)
		{
			// ASSET VALIDATION BEGIN
			if (bExternalPackage)
			{
				DataValidationLog.Message(Severity)
				->AddToken(FTextToken::Create(FText::FromString(TEXT("[AssetLog]")))) // asset log prefix
				->AddToken(FUObjectToken::Create(Asset)) // asset token
				->AddToken(FTextToken::Create(FText::FromString(TEXT(":")))) // bruh this colon man
				->AddToken(FTextToken::Create(Msg)); // actual message
			}
			// ASSET VALIDATION END
			else
			{
				const FString AssetLogString = FAssetMsg::GetAssetLogString(*PackageName, Msg.ToString());
				
				FString BeforeAsset{}, AfterAsset{};
				TSharedRef<FTokenizedMessage> TokenizedMessage = DataValidationLog.Message(Severity);
				if (AssetLogString.Split(PackageName, &BeforeAsset, &AfterAsset))
				{
					if (!BeforeAsset.IsEmpty())
					{
						TokenizedMessage->AddToken(FTextToken::Create(FText::FromString(BeforeAsset)));
					}
				
					TokenizedMessage->AddToken(FAssetNameToken::Create(PackageName));
				
					// ASSET VALIDATION BEGIN
					// add asset name to AfterAsset for cases like default object validation and asset validation that are not yet on disk
					FString AssetNameString = AssetData.AssetName.ToString();
					AssetNameString.RemoveFromStart(TEXT("Default__"));
					AfterAsset.InsertAt(0, AssetNameString);
					// ASSET VALIDATION END
				
					if (!AfterAsset.IsEmpty())
					{
						TokenizedMessage->AddToken(FTextToken::Create(FText::FromString(AfterAsset)));
					}
				}
				else
				{
					TokenizedMessage->AddToken(FTextToken::Create(FText::FromString(AssetLogString)));
				}
			}
		}
	};
	
	// Now add to map or update as needed
	for (const FAssetData& AssetData : AssetDataList)
	{
		FText ValidatingMessage = FText::Format(LOCTEXT("ValidatingFilename", "Validating {0}"), FText::FromString(AssetData.GetFullName()));
		SlowTask.EnterProgressFrame(1.0f / NumFilesToValidate, ValidatingMessage);

		if (AssetData.HasAnyPackageFlags(PKG_Cooked))
		{
			++NumSkippedFiles;
			continue;
		}

		// Check exclusion path
		if (InSettings.bSkipExcludedDirectories && IsPathExcludedFromValidation(AssetData.PackageName.ToString()))
		{
			++NumSkippedFiles;
			continue;
		}

		const bool bLoadAsset = false;
		if (!InSettings.bLoadAssetsForValidation && !AssetData.FastGetAsset(bLoadAsset))
		{
			++NumSkippedFiles;
			continue;
		}

		UE_LOG(LogAssetValidation, Display, TEXT("%s"), *ValidatingMessage.ToString());

		TArray<FText> ValidationErrors;
		TArray<FText> ValidationWarnings;

		const int32 CachedValidatedAssetsCount = ValidatedAssetsCount;
		auto CachedValidationResults = ValidationResults;
		EDataValidationResult Result = IsAssetValid(AssetData, ValidationErrors, ValidationWarnings, InSettings.ValidationUsecase);

		// ASSET VALIDATION BEGIN
		// updated Checked, Valid, Invalid and Not Validated counters
		NumFilesChecked += ValidatedAssetsCount - CachedValidatedAssetsCount;
		NumInvalidFiles += ValidationResults[0] - CachedValidationResults[0];
		NumValidFiles += ValidationResults[1] - CachedValidationResults[1];
		NumFilesUnableToValidate += ValidationResults[2] - CachedValidationResults[2];
		// ASSET VALIDATION END
		
		AddAssetLogTokens(EMessageSeverity::Error, ValidationErrors, AssetData);

		if (ValidationWarnings.Num() > 0)
		{
			++NumFilesWithWarnings; // @todo: this is incorrect, but currently there's no way to tell how many assets validated with warnings
			AddAssetLogTokens(EMessageSeverity::Warning, ValidationWarnings, AssetData);
		}

		// ASSET VALIDATION BEGIN
		if (Result == EDataValidationResult::Valid)
		{
			if (ValidationWarnings.Num() > 0)
			{
				// ASSET VALIDATION BEGIN
				DataValidationLog.Info()->AddToken(GetAssetToken(AssetData))
				// ASSET VALIDATION END
					->AddToken(FTextToken::Create(LOCTEXT("ContainsWarningsResult", "contains valid data, but has warnings.")));
			}
		}
		else if (Result == EDataValidationResult::Invalid)
		{
			// ASSET VALIDATION BEGIN
			DataValidationLog.Info()->AddToken(GetAssetToken(AssetData))
			// ASSET VALIDATION END
				->AddToken(FTextToken::Create(LOCTEXT("InvalidDataResult", "contains invalid data.")));
		}
		else if (Result == EDataValidationResult::NotValidated)
		{
			if (InSettings.bShowIfNoFailures)
			{
				// ASSET VALIDATION BEGIN
				DataValidationLog.Info()->AddToken(GetAssetToken(AssetData))
				// ASSET VALIDATION END
					->AddToken(FTextToken::Create(LOCTEXT("NotValidatedDataResult", "has no data validation.")));
			}
		}

		if (InSettings.bCollectPerAssetDetails)
		{
			FValidateAssetsDetails& Details = OutResults.AssetsDetails.Emplace(AssetData.GetObjectPathString());
			Details.PackageName = AssetData.PackageName;
			Details.AssetName = AssetData.AssetName;
			Details.Result = Result;
			Details.ValidationErrors = MoveTemp(ValidationErrors);
			Details.ValidationWarnings = MoveTemp(ValidationWarnings);
		}
	}

	const bool bFailed = (NumInvalidFiles > 0);
	const bool bAtLeastOneWarning = (NumFilesWithWarnings > 0);

	OutResults.NumChecked = NumFilesChecked;
	OutResults.NumValid = NumValidFiles;
	OutResults.NumInvalid = NumInvalidFiles;
	OutResults.NumSkipped = NumSkippedFiles;
	OutResults.NumWarnings = NumFilesWithWarnings;
	OutResults.NumUnableToValidate = NumFilesUnableToValidate;

	if (bFailed || bAtLeastOneWarning || InSettings.bShowIfNoFailures)
	{
		FFormatNamedArguments Arguments;
		Arguments.Add(TEXT("Result"), bFailed ? LOCTEXT("Failed", "FAILED") : LOCTEXT("Succeeded", "SUCCEEDED"));
		Arguments.Add(TEXT("NumChecked"), NumFilesChecked);
		Arguments.Add(TEXT("NumValid"), NumValidFiles);
		Arguments.Add(TEXT("NumInvalid"), NumInvalidFiles);
		Arguments.Add(TEXT("NumSkipped"), NumSkippedFiles);
		Arguments.Add(TEXT("NumUnableToValidate"), NumFilesUnableToValidate);

		DataValidationLog.Info()->AddToken(FTextToken::Create(FText::Format(LOCTEXT("SuccessOrFailure", "Data validation {Result}."), Arguments)));
		DataValidationLog.Info()->AddToken(FTextToken::Create(FText::Format(LOCTEXT("ResultsSummary", "Files Checked: {NumChecked}, Passed: {NumValid}, Failed: {NumInvalid}, Skipped: {NumSkipped}, Unable to validate: {NumUnableToValidate}"), Arguments)));

		DataValidationLog.Open(EMessageSeverity::Info, true);
	}

	// Broadcast now that we're complete so other systems can go back to their previous state.
	if (FEditorDelegates::OnPostAssetValidation.IsBound())
	{
		FEditorDelegates::OnPostAssetValidation.Broadcast();
	}

	return NumInvalidFiles + NumFilesWithWarnings;
}

EDataValidationResult UAssetValidationSubsystem::IsAssetValid(const FAssetData& AssetData, TArray<FText>& ValidationErrors, TArray<FText>& ValidationWarnings, const EDataValidationUsecase InValidationUsecase) const
{
	EDataValidationResult Result = EDataValidationResult::Valid; // @todo: for some reason EDataValidationResult::NotValidated returns always NotValidated
	if (AssetData.FastGetAsset() != nullptr || ShouldLoadAsset(AssetData))
	{
		// call default implementation that loads an asset and calls IsObjectValid
		Result = Super::IsAssetValid(AssetData, ValidationErrors, ValidationWarnings, InValidationUsecase);
	}
	else
	{
		++ValidatedAssetsCount; // explicitly increase validated assets count
		for (auto& ValidatorPair: Validators)
		{
			if (UAssetValidator* Validator = Cast<UAssetValidator>(ValidatorPair.Value))
			{
				if (CanUseValidator(Validator, InValidationUsecase))
				{
					Validator->ResetValidationState();
					// attempt to validate asset data. Asset validator may or may not load the asset in question
					Result &= Validator->ValidateAsset(AssetData, ValidationErrors);

					ValidationWarnings.Append(Validator->GetAllWarnings());
				}
			}
		}
	}

	ValidationResults[static_cast<uint8>(Result)] += 1;
	return Result;
}

EDataValidationResult UAssetValidationSubsystem::IsObjectValid(UObject* InObject, TArray<FText>& ValidationErrors, TArray<FText>& ValidationWarnings, const EDataValidationUsecase InValidationUsecase) const
{
	if (!CanValidateAsset(InObject))
	{
		// return that asset was not validated
		return EDataValidationResult::NotValidated;
	}

	++ValidatedAssetsCount; // explicitly increase validated assets count
	return Super::IsObjectValid(InObject, ValidationErrors, ValidationWarnings, InValidationUsecase);
}

bool UAssetValidationSubsystem::CanValidateAsset(UObject* Asset) const
{
	if (Asset == nullptr)
	{
		return false;
	}
	
	const FString PackageName = Asset->GetPackage()->GetName();
	const UProjectPackagingSettings* PackagingSettings = GetDefault<UProjectPackagingSettings>();

	for (const FDirectoryPath& Directory: PackagingSettings->DirectoriesToNeverCook)
	{
		const FString& Folder = Directory.Path;
		if (PackageName.StartsWith(Folder))
		{
			return false;
		}
	}

	if (PackageName.StartsWith(TEXT("/Game/Developers/")))
	{
		return false;
	}

	return true;
}

bool UAssetValidationSubsystem::ShouldLoadAsset(const FAssetData& AssetData) const
{
	// don't load maps, map data or cooked packages
	return !AssetData.HasAnyPackageFlags(PKG_ContainsMap | PKG_ContainsMapData | PKG_Cooked);
}

bool UAssetValidationSubsystem::CanUseValidator(const UEditorValidatorBase* Validator, EDataValidationUsecase Usecase) const
{
	return Validator && Validator->IsEnabled() && Validator->CanValidate(Usecase);
}

EDataValidationResult UAssetValidationSubsystem::IsActorValid(AActor* Actor, FDataValidationContext& Context) const
{
	// This is a rough copy of a AActor::IsDataValid implementation, which has several reasons to exist:
	// - AActor::IsDataValid early outs on external actors (hello WP), which means actor components are skipped @todo verify
	// - AActor::IsDataValid calls CheckForErrors and if any warnings are encountered adds an error to validation context.
	// On the contrary, we don't want to call CheckForErrors, as map check is probably run somewhere in automation pipeline
	// or is expressed by another asset validator (that doesn't work with actors)
	// RF_HasExternalPackage flag allows us to early out in AActor::IsDataValid
	const bool bHasExternalPackage = Actor->HasAnyFlags(RF_HasExternalPackage);
	Actor->SetFlags(RF_HasExternalPackage);

	EDataValidationResult Result = EDataValidationResult::Valid;
	// run actual actor validation, skipping AActor::IsValid implementation
	Result &= Actor->IsDataValid(Context);

	// run default subobject validation
	if (CheckDefaultSubobjects() == false)
	{
		Result = EDataValidationResult::Invalid;
		const FText ErrorMsg = FText::Format(NSLOCTEXT("ErrorChecking", "IsDataValid_Failed_CheckDefaultSubobjectsInternal", "{0} failed CheckDefaultSubobjectsInternal()"), FText::FromString(GetName()));
		Context.AddError(ErrorMsg);
	}

	// validate actor components
	for (const UActorComponent* Component : Actor->GetComponents())
	{
		if (Component)
		{
			// if any component is invalid, our result is invalid
			// in the future we may want to update this to say that the actor was not validated if any of its components returns EDataValidationResult::NotValidated
			Result &= Component->IsDataValid(Context);
		}
	}

	if (!bHasExternalPackage)
	{
		// fix RF_HasExternalPackage flag
		Actor->ClearFlags(RF_HasExternalPackage);
	}

	return Result;
}

#undef LOCTEXT_NAMESPACE