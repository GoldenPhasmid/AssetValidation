#include "AssetValidationSubsystem.h"

#include "AssetValidationModule.h"
#include "AssetValidationStatics.h"
#include "DataValidationChangelist.h"
#include "EditorValidatorBase.h"
#include "EditorValidatorHelpers.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "SourceControlProxy.h"
#include "Algo/AnyOf.h"
#include "AssetRegistry/AssetDataToken.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetValidators/AssetValidator.h"
#include "Misc/DataValidation.h"
#include "Misc/ScopedSlowTask.h"
#include "Misc/UObjectToken.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

static_assert(static_cast<uint8>(EDataValidationResult::Invalid)		== 0);
static_assert(static_cast<uint8>(EDataValidationResult::Valid)			== 1);
static_assert(static_cast<uint8>(EDataValidationResult::NotValidated)	== 2);

UAssetValidationSubsystem::UAssetValidationSubsystem()
{
#if !WITH_DATA_VALIDATION_UPDATE // from 5.4 subsystem can be subclassed
	if (HasAllFlags(RF_ClassDefaultObject))
	{
		// this is a dirty hack to disable UEditorValidatorSubsystem creation and using this subsystem instead
		// I'm amazed that it actually works and doesn't break anything
		// there's no easy way in unreal to disable engine subsystems unless there's explicit support in ShouldCreateSubsystem
		UClass* Class = const_cast<UClass*>(UEditorValidatorSubsystem::StaticClass());
		Class->ClassFlags |= CLASS_Abstract;
	}
#endif
}

void UAssetValidationSubsystem::MarkPackageLoaded(const FName& PackageName)
{
	check(!IsPackageAlreadyLoaded(PackageName));
	
	UAssetValidationSubsystem* ValidationSubsystem = Get();
	check(ValidationSubsystem);
	
	ValidationSubsystem->LoadedPackageNames.Add(PackageName);
}

bool UAssetValidationSubsystem::IsPackageAlreadyLoaded(const FName& PackageName)
{
	UAssetValidationSubsystem* ValidationSubsystem = Get();
	check(ValidationSubsystem);
	return ValidationSubsystem->LoadedPackageNames.Contains(PackageName);
}	

#if WITH_DATA_VALIDATION_UPDATE
int32 UAssetValidationSubsystem::ValidateAssetsWithSettings(const TArray<FAssetData>& AssetDataList, FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults) const
{
	checkf(bRecursiveCall == false, TEXT("%s: can't handle recursive calls."), *FString(__FUNCTION__));
	TGuardValue RecursionGuard{bRecursiveCall, true};

	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(AssetValidationSubsystem_ValidateAssetsWithSettings, AssetValidationChannel);
	
	// reset before validation in case of further IsAssetValid/IsObjectValid requests
	ResetValidationState();

	FMessageLog DataValidationLog{UE::DataValidation::MessageLogName};
	if (InSettings.ValidationUsecase != EDataValidationUsecase::Save)
	{
		// epic's forgot to open a new page when validation is manual
		// very useful "DataValidation refactor", thanks
		DataValidationLog.NewPage(InSettings.MessageLogPageTitle);
	}

	CurrentSettings = TOptional{InSettings};
	FValidateAssetsResults PrevResults = OutResults;
	
	ValidateAssetsResolverInternal(DataValidationLog, AssetDataList, InSettings, OutResults);

	// override asset count calculation to account for recursive validation
	// also include previous results in case @OutResults was used more than once
	OutResults.NumRequested			= PrevResults.NumRequested + AssetDataList.Num();
	OutResults.NumChecked			= PrevResults.NumChecked + CheckedAssetsCount;
	OutResults.NumValid				= PrevResults.NumValid + ValidationResults[static_cast<uint8>(EDataValidationResult::Valid)];
	OutResults.NumInvalid			= PrevResults.NumInvalid + ValidationResults[static_cast<uint8>(EDataValidationResult::Invalid)];
	OutResults.NumUnableToValidate	= PrevResults.NumUnableToValidate + ValidationResults[static_cast<uint8>(EDataValidationResult::NotValidated)];
	
	LogAssetValidationSummary(DataValidationLog, InSettings, OutResults);

	// reset after validation in case of further IsAssetValid/IsObjectValid requests
	ResetValidationState();
	
	return OutResults.NumWarnings + OutResults.NumInvalid;
}
#endif

#if WITH_DATA_VALIDATION_UPDATE
EDataValidationResult UAssetValidationSubsystem::ValidateChangelist(UDataValidationChangelist* InChangelist, const FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults) const
{
	return ValidateChangelists({InChangelist}, InSettings, OutResults);
}

EDataValidationResult UAssetValidationSubsystem::ValidateChangelists(const TArray<UDataValidationChangelist*> InChangelists, const FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults) const
{
	checkf(bRecursiveCall == false, TEXT("%s: can't handle recursive calls."), *FString(__FUNCTION__));
	TGuardValue RecursionGuard{bRecursiveCall, true};

	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(AssetValidationSubsystem_ValidateChangelists, AssetValidationChannel);
	
	ResetValidationState();

	if (InSettings.ValidationUsecase != EDataValidationUsecase::Save)
	{
		// epic's forgot to open a new page when validation is manual
		// very useful "DataValidation refactor", thanks
		FMessageLog DataValidationLog(UE::DataValidation::MessageLogName);
		DataValidationLog.SetCurrentPage(InSettings.MessageLogPageTitle);
	}
	
	EDataValidationResult Result = Super::ValidateChangelists(InChangelists, InSettings, OutResults);

	// override asset count calculation to account for recursive validation
	OutResults.NumChecked = CheckedAssetsCount;
	OutResults.NumValid = ValidationResults[static_cast<uint8>(EDataValidationResult::Valid)];
	OutResults.NumInvalid = ValidationResults[static_cast<uint8>(EDataValidationResult::Invalid)];
	OutResults.NumUnableToValidate = ValidationResults[static_cast<uint8>(EDataValidationResult::NotValidated)];

	FMessageLog DataValidationLog(UE::DataValidation::MessageLogName);
	LogAssetValidationSummary(DataValidationLog, InSettings, OutResults);
	
	return Result;
}

void UAssetValidationSubsystem::GatherAssetsToValidateFromChangelist(UDataValidationChangelist* InChangelist, const FValidateAssetsSettings& Settings, TSet<FAssetData>& OutAssets, FDataValidationContext& InContext) const
{
	if (IsEmptyChangelist(InChangelist))
	{
		auto SCProxy = IAssetValidationModule::Get().GetSourceControlProxy();

		TArray<FSourceControlStateRef> OpenedFiles;
		SCProxy->GetOpenedFiles(OpenedFiles);

		InChangelist->Initialize(OpenedFiles);
	}
	
	Super::GatherAssetsToValidateFromChangelist(InChangelist, Settings, OutAssets, InContext);

	// @todo: source file analysis
}
#endif

bool UAssetValidationSubsystem::IsEmptyChangelist(UDataValidationChangelist* Changelist) const
{
	if (Changelist->Changelist.IsValid())
	{
		ISourceControlProvider& Provider = ISourceControlModule::Get().GetProvider();
		FSourceControlChangelistStatePtr ChangelistStatePtr = Provider.GetState(Changelist->Changelist.ToSharedRef(), EStateCacheUsage::Use);
		return ChangelistStatePtr.IsValid();
	}

	bool bEmpty = Changelist->ModifiedPackageNames.Num() & 0x01;
	bEmpty &= Changelist->DeletedPackageNames.Num() & 0x01;
	bEmpty &= Changelist->ModifiedFiles.Num() & 0x01;
	bEmpty &= Changelist->DeletedFiles.Num() & 0x01;
	return bEmpty;
}

void UAssetValidationSubsystem::ValidateAssetsResolverInternal(FMessageLog& DataValidationLog,
	const TArray<FAssetData>& AssetDataList, const FValidateAssetsSettings& InSettings,
	FValidateAssetsResults& OutResults) const
{
	bool bCustomValidateAssets = true;
	if (bCustomValidateAssets)
	{
		ValidateAssetsInternal(DataValidationLog, AssetDataList, InSettings, OutResults);
	}
	else
	{
		Super::ValidateAssetsInternal(DataValidationLog, TSet<FAssetData>{AssetDataList}, InSettings, OutResults);
	}
}

void UAssetValidationSubsystem::ValidateAssetsInternal(
	FMessageLog& DataValidationLog,
	TArray<FAssetData> AssetDataList,
	const FValidateAssetsSettings& InSettings,
	FValidateAssetsResults& OutResults) const
{
	/**
	 *	This function is a copy of a UEditorValidatorSubsystem::ValidateAssetsInternal, but fixes several things
	 *  that were poorly implemented in the original:
	 *  1.	New abysmal logging. Before, each error/warning was logged using separate tokenized message. In new implementation,
	 *		all errors/warnings produced by (unrelated) validators are crammed into one message.
	 *		This results with error message being bloated and inproperly aligned in asset check window.
	 *		
	 *	5.	AssetDataList is TSet instead of TArray. However, original iterates twice over its members and doesn't use
	 *		TSet features like fast search.
	 *	
	 *
	 *	Unfortunately, I can't override ValidateAssetsInternal.
	 *	Thankfully, all usages of UEditorValidatorSubsystem::ValidateAssetsInternal can be overriden.
	 *	All changes are highlighted with ASSET VALIDATION BEGIN/END scope comments with purpose stated below or
	 *	as a part of scope comment.
	 *
	 *	P.S.: at this point I'm wondering if it would be easier to drop DataValidation completely
	 *	and implement everything from scratch. Let's see if next update can push me to this decision.
	 */
	
	IAssetRegistry& AssetRegistry = IAssetRegistry::GetChecked();

	FScopedSlowTask SlowTask(AssetDataList.Num(), LOCTEXT("ValidateAssetsTask", "Validating Assets"));
	SlowTask.MakeDialog();
	
	UE_LOG(LogAssetValidation, Display, TEXT("Starting to validate %d assets"), AssetDataList.Num());
	UE_LOG(LogAssetValidation, Log, TEXT("Enabled validators:"));
	ForEachEnabledValidator([](UEditorValidatorBase* Validator)
	{
		UE_LOG(LogAssetValidation, Log, TEXT("\t%s"), *Validator->GetClass()->GetClassPathName().ToString());
		return true;
	});
	
	// Broadcast the Editor event before we start validating. This lets other systems (such as Sequencer) restore the state
	// of the level to what is actually saved on disk before performing validation.
	if (FEditorDelegates::OnPreAssetValidation.IsBound())
	{
		FEditorDelegates::OnPreAssetValidation.Broadcast();
	}
	
	// Filter external objects out from the asset data list to be validated indirectly via their outers
	// ASSET VALIDATION BEGIN replace set iteration with array iteration
	TMap<FAssetData, TArray<FAssetData>> AssetsToExternalObjects;
	for (int32 Index = AssetDataList.Num() - 1; Index >= 0; --Index)
	{
		const FAssetData& AssetData = AssetDataList[Index];
		if (!AssetData.GetOptionalOuterPathName().IsNone())
		{
			FSoftObjectPath Path = AssetData.ToSoftObjectPath();
			FAssetData OuterAsset = AssetRegistry.GetAssetByObjectPath(Path.GetWithoutSubPath(), true);
			if (OuterAsset.IsValid())
			{
				AssetsToExternalObjects.FindOrAdd(OuterAsset).Add(AssetData);
			}
			AssetDataList.RemoveAtSwap(Index);
		}
	}
	// ASSET VALIDATION END
	
	// Add any packages which contain those external objects to be validated
	{
		FDataValidationContext ValidationContext(false, InSettings.ValidationUsecase, {});
		for (const TPair<FAssetData, TArray<FAssetData>>& Pair : AssetsToExternalObjects)
		{
			if (ShouldValidateAsset(Pair.Key, InSettings, ValidationContext))
			{
				AssetDataList.Add(Pair.Key);
			}
		}
		UE::AssetValidation::AppendAssetValidationMessages(DataValidationLog, ValidationContext);
		DataValidationLog.Flush();
	}

	// Dont let other async compilation warnings be attributed incorrectly to the package that is loading.
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(AssetValidation_WaitAssetCompilation, AssetValidationChannel);
		WaitForAssetCompilationIfNecessary(InSettings.ValidationUsecase);
	}
	
	OutResults.NumRequested = AssetDataList.Num();
	
	// Now add to map or update as needed
	for (const FAssetData& AssetData : AssetDataList)
	{
		ensure(AssetData.IsValid());

		SlowTask.EnterProgressFrame(1.0f, FText::Format(LOCTEXT("ValidatingFilename", "Validating {0}"), FText::FromString(AssetData.GetFullName())));
		
		if (OutResults.NumChecked >= InSettings.MaxAssetsToValidate)
		{
			OutResults.bAssetLimitReached = true;
			DataValidationLog.Info(FText::Format(LOCTEXT("AssetLimitReached", "MaxAssetsToValidate count {0} reached."), InSettings.MaxAssetsToValidate));
			break;
		}

		if (AssetData.HasAnyPackageFlags(PKG_Cooked))
		{
			++OutResults.NumSkipped;
			continue;
		}

		// Check exclusion path
		if (InSettings.bSkipExcludedDirectories && IsPathExcludedFromValidation(AssetData.PackageName.ToString()))
		{
			++OutResults.NumSkipped;
			continue;
		}

		const bool bLoadAsset = false;
		if (!InSettings.bLoadAssetsForValidation && !AssetData.FastGetAsset(bLoadAsset))
		{
			++OutResults.NumSkipped;
			continue;
		}

// ASSET VALIDATION BEGIN remove this god forsaken logging ALWAYS for EVERY asset
		if (bLogEveryAssetBecauseYouWantYourLogThrashed)
		{
			DataValidationLog.Info()
			->AddToken(UE::AssetValidation::CreateAssetToken(AssetData))
			->AddToken(FTextToken::Create(LOCTEXT("ValidatingAsset", "Validating asset")));
			UE_LOG(LogAssetValidation, Display, TEXT("Validating asset %s"), *AssetData.ToSoftObjectPath().ToString());
		}
// ASSET VALIDATION END
		
		UObject* LoadedAsset = AssetData.FastGetAsset(false);
		const bool bAlreadyLoaded = LoadedAsset != nullptr;

		TConstArrayView<FAssetData> ValidationExternalObjects;
		if (const TArray<FAssetData>* ValidationExternalObjectsPtr = AssetsToExternalObjects.Find(AssetData))
		{
			ValidationExternalObjects = *ValidationExternalObjectsPtr;
		}

// ASSET VALIDATION BEGIN fix epic's bug that bWasAssetLoadedForValidation == bAlreadyLoaded. Should be the opposite
		FDataValidationContext ValidationContext(!bAlreadyLoaded, InSettings.ValidationUsecase, ValidationExternalObjects);
// ASSET VALIDATION END

// ASSET VALIDATION BEGIN move asset load functionality to IsAssetValidWithContext
		EDataValidationResult Result = IsAssetValidWithContext(AssetData, ValidationContext);
// ASSET VALIDATION END
		
		// Don't add more messages to ValidationContext after this point because we will no longer add them to the message log
		UE::AssetValidation::AppendAssetValidationMessages(DataValidationLog, AssetData, ValidationContext);

		const bool bAnyWarnings = ValidationContext.GetNumWarnings() > 0;

		if (Result == EDataValidationResult::Valid)
		{
			if (bAnyWarnings)
			{
				DataValidationLog.Info()
				->AddToken(UE::AssetValidation::CreateAssetToken(AssetData))
				->AddToken(FTextToken::Create(LOCTEXT("ContainsWarningsResult", "contains valid data, but has warnings.")));
			}
		}
		else if (Result == EDataValidationResult::Invalid)
		{
			DataValidationLog.Info()
			->AddToken(UE::AssetValidation::CreateAssetToken(AssetData))
			->AddToken(FTextToken::Create(LOCTEXT("InvalidDataResult", "contains invalid data.")));
		}
		else if (Result == EDataValidationResult::NotValidated)
		{
			if (InSettings.bShowIfNoFailures)
			{
				DataValidationLog.Info()
				->AddToken(UE::AssetValidation::CreateAssetToken(AssetData))
				->AddToken(FTextToken::Create(LOCTEXT("NotValidatedDataResult", "has no data validation.")));
			}
		}
		
		if (InSettings.bCollectPerAssetDetails)
		{
			FValidateAssetsDetails& Details = OutResults.AssetsDetails.Emplace(AssetData.GetObjectPathString());
			Details.PackageName = AssetData.PackageName;
			Details.AssetName = AssetData.AssetName;
			Details.Result = Result;
			ValidationContext.SplitIssues(Details.ValidationWarnings, Details.ValidationErrors);

			Details.ExternalObjects.Reserve(ValidationExternalObjects.Num());
			for (const FAssetData& ExtData : ValidationExternalObjects)
			{
				FValidateAssetsExternalObject& ExtDetails = Details.ExternalObjects.Emplace_GetRef();
				ExtDetails.PackageName = ExtData.PackageName;
				ExtDetails.AssetName = ExtData.AssetName;
			}
		}
		
		DataValidationLog.Flush();
	}

	// Broadcast now that we're complete so other systems can go back to their previous state.
	if (FEditorDelegates::OnPostAssetValidation.IsBound())
	{
		FEditorDelegates::OnPostAssetValidation.Broadcast();
	}
}

EDataValidationResult UAssetValidationSubsystem::IsAssetValidWithContext(const FAssetData& AssetData, FDataValidationContext& InContext) const
{
	EDataValidationResult Result = EDataValidationResult::NotValidated;
	if (!AssetData.IsValid())
	{
		return Result;
	}
	
	if (!CurrentSettings.IsSet())
	{
		// @todo: use settings from AssetValidationSettings
		CurrentSettings = TOptional{FValidateAssetsSettings{}};
	}

	// explicitly increase validated assets count
	++CheckedAssetsCount; 
	
	const UObject* Asset = AssetData.FastGetAsset(false);
	if (Asset == nullptr && ShouldLoadAsset(Asset))
	{
		UE_LOG(LogAssetValidation, Verbose, TEXT("Loading asset %s for validation"), *AssetData.ToSoftObjectPath().ToString());
		UE::DataValidation::FScopedLogMessageGatherer LogGatherer{CurrentSettings->bCaptureAssetLoadLogs};
		Asset = AssetData.GetAsset(); // Do not load external objects, validators should load external objects that they want via GetAssociatedExternalObjects in the validation context 

		{
			TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(AssetValidation_WaitAssetCompilation, AssetValidationChannel);
			WaitForAssetCompilationIfNecessary(InContext.GetValidationUsecase());
		}

		// make a human readable append to data validation context. @see UEditorValidatorSubsystem.cpp 504
		// Associate any load errors with this asset in the message log
		UE::AssetValidation::AppendAssetValidationMessages(InContext, AssetData, LogGatherer);

		MarkPackageLoaded(AssetData.PackageName);
	}
	
	if (Asset)
	{
		// call default implementation that loads an asset and calls IsObjectValid
		UE::DataValidation::FScopedLogMessageGatherer LogGatherer(CurrentSettings->bCaptureLogsDuringValidation);
		Result &= Super::IsAssetValidWithContext(AssetData, InContext);
	}
	else
	{
		ForEachEnabledValidator([&Result, &AssetData, &InContext](UEditorValidatorBase* Validator)
		{
			if (UAssetValidator* AssetValidator = Cast<UAssetValidator>(Validator))
			{
				AssetValidator->ResetValidationState();
				if (AssetValidator->K2_CanValidate(InContext.GetValidationUsecase()) && AssetValidator->CanValidateAsset_Implementation(AssetData, nullptr, InContext))
				{
					// attempt to validate asset data. Asset validator may or may not load the asset in question
					Result &= AssetValidator->ValidateAsset(AssetData, InContext);
				}
			}
			return true;
		});
	}

	ValidationResults[static_cast<uint8>(Result)] += 1;
	return Result;
}

EDataValidationResult UAssetValidationSubsystem::IsObjectValidWithContext(UObject* InAsset, FDataValidationContext& InContext) const
{
	EDataValidationResult Result = EDataValidationResult::NotValidated;
	if (InAsset)
	{
		++CheckedAssetsCount; // explicitly increase validated assets count
		Result = Super::IsObjectValidWithContext(InAsset, InContext);
	}

	ValidationResults[static_cast<uint8>(Result)] += 1;
	return Result;
}

bool UAssetValidationSubsystem::ShouldValidateAsset(const FAssetData& Asset, const FValidateAssetsSettings& Settings, FDataValidationContext& InContext) const
{
	return UE::AssetValidation::ShouldValidatePackage(Asset.PackageName.ToString()) && Super::ShouldValidateAsset(Asset, Settings, InContext);
}

bool UAssetValidationSubsystem::ShouldLoadAsset(const FAssetData& AssetData) const
{
	// don't load maps, map data or cooked packages
	return !AssetData.HasAnyPackageFlags(PKG_ContainsMap | PKG_ContainsMapData | PKG_Cooked);
}

void UAssetValidationSubsystem::ResetValidationState() const
{
	const_cast<UAssetValidationSubsystem&>(*this).ResetValidationState();
}

void UAssetValidationSubsystem::ResetValidationState()
{
	CheckedAssetsCount = 0;
	LoadedPackageNames.Empty(32);
	FMemory::Memzero(ValidationResults.GetData(), ValidationResults.Num() * sizeof(int32));

	CurrentSettings.Reset();
}

#if !WITH_DATA_VALIDATION_UPDATE // world actor validation was fixed in 5.4
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

		const int32 CachedValidatedAssetsCount = CheckedAssetsCount;
		auto CachedValidationResults = ValidationResults;
		EDataValidationResult Result = IsAssetValid(AssetData, ValidationErrors, ValidationWarnings, InSettings.ValidationUsecase);

		// ASSET VALIDATION BEGIN
		// updated Checked, Valid, Invalid and Not Validated counters
		NumFilesChecked += CheckedAssetsCount - CachedValidatedAssetsCount;
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

	return OutResults.NumInvalid + OutResults.NumWarnings;
}
#endif // world actor validation was fixed in 5.4

#if !WITH_DATA_VALIDATION_UPDATE // 5.4 CheckForErrors was removed from AActor::IsDataValid which makes separate actor validation obsolete
EDataValidationResult UAssetValidationSubsystem::IsStandaloneActorValid(AActor* Actor, FDataValidationContext& Context) const
{
	EDataValidationResult Result = EDataValidationResult::NotValidated;
	if (Actor == nullptr)
	{
		return Result;
	}
	
	++ValidatedAssetsCount;
	// similar flow to UEditorValidatorSubsystem::ValidateObjectInternal
	if (Actor != nullptr)
	{
		Result = IsActorValid(Actor, Context);
		if (Result == EDataValidationResult::Invalid)
		{
			return Result;
		}
		
		FAssetData ActorData{Actor};
		ForEachEnabledValidator([Actor, &ActorData, &Context, &Result](UEditorValidatorBase* Validator)
		{
			Result &= Validator->ValidateLoadedAsset(ActorData, Actor, Context);
			return true;
		});
	}

	ValidationResults[static_cast<uint8>(Result)] += 1;
	return Result;
}
#endif

#if !WITH_DATA_VALIDATION_UPDATE // 5.4 CheckForErrors was removed from AActor::IsDataValid which makes separate actor validation obsolete
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
#endif

#undef LOCTEXT_NAMESPACE