#include "AssetValidationSubsystem.h"

#include "AssetValidationDefines.h"
#include "AssetValidationModule.h"
#include "AssetValidationSettings.h"
#include "AssetValidationStatics.h"
#include "DataValidationChangelist.h"
#include "EditorValidatorBase.h"
#include "EditorValidatorHelpers.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "SourceControlProxy.h"
#include "Algo/RemoveIf.h"
#include "AssetRegistry/AssetDataToken.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetValidators/AssetValidator.h"
#include "Misc/DataValidation.h"
#include "Misc/ScopedSlowTask.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

static_assert(static_cast<uint8>(EDataValidationResult::Invalid)		== 0);
static_assert(static_cast<uint8>(EDataValidationResult::Valid)			== 1);
static_assert(static_cast<uint8>(EDataValidationResult::NotValidated)	== 2);

UAssetValidationSubsystem::UAssetValidationSubsystem()
{
}

void UAssetValidationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ForEachEnabledValidator([this](UEditorValidatorBase* EditorValidator)
	{
		if (UAssetValidator* AssetValidator = Cast<UAssetValidator>(EditorValidator))
		{
			if (AssetValidator->CanValidateActors())
			{
				ActorValidators.Add(AssetValidator);
			}
		}

		return true;
	});
}

void UAssetValidationSubsystem::Deinitialize()
{
	ActorValidators.Empty();
	
	Super::Deinitialize();
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

int32 UAssetValidationSubsystem::ValidateAssetsWithSettings(const TArray<FAssetData>& AssetDataList, FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults) const
{
	checkf(bRecursiveCall == false, TEXT("%s: can't handle recursive calls."), *FString(__FUNCTION__));
	TGuardValue RecursionGuard{bRecursiveCall, true};

	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(AssetValidationSubsystem_ValidateAssetsWithSettings, AssetValidationChannel);
	
	// reset before validation in case of further IsAssetValid/IsObjectValid requests
	ResetValidationState();

	FMessageLog DataValidationLog{UE::DataValidation::MessageLogName};
	// do not open log message for save and script validation use case. Save is handled by ValidateOnSave, Script is handled separately
	if (InSettings.ValidationUsecase != EDataValidationUsecase::Save && InSettings.ValidationUsecase != EDataValidationUsecase::Script) 
	{
		// epic's forgot to open a new page when validation is manual
		// very useful "DataValidation refactor", thanks
		DataValidationLog.NewPage(InSettings.MessageLogPageTitle);
	}
	DataValidationLog.Message(EMessageSeverity::Info, InSettings.MessageLogPageTitle);

	CurrentSettings = TOptional{InSettings};
	FValidateAssetsResults PrevResults = OutResults;
	
	ValidateAssetsInternalResolver(DataValidationLog, AssetDataList, InSettings, OutResults);

	// override asset count calculation to account for recursive validation
	// also include previous results in case @OutResults was used more than once
	OutResults.NumRequested			= PrevResults.NumRequested + AssetDataList.Num();
	OutResults.NumChecked			= PrevResults.NumChecked + CheckedAssetsCount;
	OutResults.NumValid				= PrevResults.NumValid + ValidationResults[static_cast<uint8>(EDataValidationResult::Valid)];
	OutResults.NumInvalid			= PrevResults.NumInvalid + ValidationResults[static_cast<uint8>(EDataValidationResult::Invalid)];
	OutResults.NumUnableToValidate	= PrevResults.NumUnableToValidate + ValidationResults[static_cast<uint8>(EDataValidationResult::NotValidated)];
	
	LogAssetValidationSummary(DataValidationLog, InSettings, OutResults);

	// request garbage collection, as some validators do very heavy lifting
	GEngine->ForceGarbageCollection(true);

	// reset after validation in case of further IsAssetValid/IsObjectValid requests
	ResetValidationState();
	
	return OutResults.NumWarnings + OutResults.NumInvalid;
}

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

	// Choose a specific message log page for this output, flushing in case other recursive calls also write to this log 
	FMessageLog DataValidationLog{UE::DataValidation::MessageLogName};
	DataValidationLog.SetCurrentPage(InSettings.MessageLogPageTitle);
	// clear log from previous validation messages on the same changelist
	UE::AssetValidation::ClearLogMessages(DataValidationLog);

	CurrentSettings = TOptional{InSettings};
	FValidateAssetsResults PrevResults = OutResults;
	
	EDataValidationResult Result = ValidateChangelistsInternal(DataValidationLog, InChangelists, InSettings, OutResults);

	// override asset count calculation to account for recursive validation
	// also include previous results in case @OutResults was used more than once
	OutResults.NumRequested			= PrevResults.NumRequested + InChangelists.Num();
	OutResults.NumChecked			= PrevResults.NumChecked + CheckedAssetsCount;
	OutResults.NumValid				= PrevResults.NumValid + ValidationResults[static_cast<uint8>(EDataValidationResult::Valid)];
	OutResults.NumInvalid			= PrevResults.NumInvalid + ValidationResults[static_cast<uint8>(EDataValidationResult::Invalid)];
	OutResults.NumUnableToValidate	= PrevResults.NumUnableToValidate + ValidationResults[static_cast<uint8>(EDataValidationResult::NotValidated)];
	
	LogAssetValidationSummary(DataValidationLog, InSettings, OutResults);
	
	// request garbage collection, as some validators do very heavy lifting
	GEngine->ForceGarbageCollection(true);

	// reset after validation in case of further IsAssetValid/IsObjectValid requests
	ResetValidationState();
	
	return Result;
}

void UAssetValidationSubsystem::GatherAssetsToValidateFromChangelist(UDataValidationChangelist* InChangelist, const FValidateAssetsSettings& Settings, TSet<FAssetData>& OutAssets, FDataValidationContext& InContext) const
{
	if (IsEmptyChangelist(InChangelist))
	{
		// fixup for git source control as git doesn't have changelists
		auto SCProxy = IAssetValidationModule::Get().GetSourceControlProxy();

		TArray<FSourceControlStateRef> OpenedFiles;
		SCProxy->GetOpenedFiles(OpenedFiles);

		InChangelist->Initialize(OpenedFiles);
	}
	
	Super::GatherAssetsToValidateFromChangelist(InChangelist, Settings, OutAssets, InContext);

	// @todo: source file analysis
}

bool UAssetValidationSubsystem::IsEmptyChangelist(UDataValidationChangelist* Changelist) const
{
	if (Changelist->Changelist.IsValid())
	{
		ISourceControlProvider& Provider = ISourceControlModule::Get().GetProvider();
		FSourceControlChangelistStatePtr ChangelistStatePtr = Provider.GetState(Changelist->Changelist.ToSharedRef(), EStateCacheUsage::Use);

		// not valid changelist state would mean source control implementation doesn't use changelists (like git)
		return !ChangelistStatePtr.IsValid();
	}

	bool bEmpty = Changelist->ModifiedPackageNames.IsEmpty();
	bEmpty &= Changelist->DeletedPackageNames.IsEmpty();
	bEmpty &= Changelist->ModifiedFiles.IsEmpty();
	bEmpty &= Changelist->DeletedFiles.IsEmpty();
	return bEmpty;
}

EDataValidationResult UAssetValidationSubsystem::ValidateAssetsInternalResolver(FMessageLog& DataValidationLog,
	const TArray<FAssetData>& AssetDataList, const FValidateAssetsSettings& InSettings,
	FValidateAssetsResults& OutResults) const
{
	constexpr bool bCustomValidateAssets = true;
	if (bCustomValidateAssets)
	{
		return ValidateAssetsInternal(DataValidationLog, AssetDataList, InSettings, OutResults);
	}
	
	return Super::ValidateAssetsInternal(DataValidationLog, TSet<FAssetData>{AssetDataList}, InSettings, OutResults);
}

EDataValidationResult UAssetValidationSubsystem::ValidateAssetsInternal(
	FMessageLog& DataValidationLog,
	TArray<FAssetData> AssetDataList,
	const FValidateAssetsSettings& InSettings,
	FValidateAssetsResults& OutResults) const
{
	/**
	 *	This function is a copy of a UEditorValidatorSubsystem::ValidateAssetsInternal, but fixes several things
	 *  that were poorly implemented in the original:
	 *  1.	New logging choice is questionable. Before, each error/warning was logged using separate tokenized message.
	 *		In new implementation, all errors/warnings produced by (unrelated) validators are crammed into one message.
	 *		This results with error message being bloated and inproperly aligned in asset check window.
	 *		Excessive logging is also an issue, I don't understand the reason behind logging every asset that is being validated.
	 *	2.	Unable to control which assets should be loaded before validtors or should be loaded as part of actual validation.
	 *		Now, IsAssetValidWithContext controls whether asset should be loaded beforehand or passed as asset data.
	 *	3.	AssetDataList is TSet instead of TArray. Original implementation iterates twice over its members and doesn't use
	 *		TSet features like fast search.
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
	SlowTask.MakeDialog(ShouldShowCancelButton(AssetDataList.Num(), InSettings));
	
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
	
	// Add external object owners to the asset data list.
	// External objects are removed and their validation should be handled by the outer assets
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
				AssetDataList.AddUnique(OuterAsset);
			}

			AssetDataList.RemoveAtSwap(Index);
		}
	}
	// ASSET VALIDATION END

	// ASSET VALIDATION BEGIN replace map iteration with array iteration
	{
		// clear assets that we shouldn't validate
		FDataValidationContext ValidationContext(false, InSettings.ValidationUsecase, {});
		AssetDataList.SetNum(Algo::RemoveIf(AssetDataList, [this, &InSettings, &ValidationContext, &DataValidationLog](const FAssetData& AssetData)
		{
			if (!ShouldValidateAsset(AssetData, InSettings, ValidationContext))
			{
				if (UAssetValidationSettings::Get()->bEnabledDetailedAssetLogging)
				{
					DataValidationLog.Info()
					->AddToken(FAssetDataToken::Create(AssetData))
					->AddToken(FTextToken::Create(LOCTEXT("ValidatingAsset", "Asset is excluded from validation, ShouldValidateAsset() returned false.")));
				}
				UE_LOG(LogAssetValidation, Verbose, TEXT("Excluding asset %s from validation"), *AssetData.GetSoftObjectPath().ToString());
				return true;
			}

			return false;
		}));
		UE::AssetValidation::AppendAssetValidationMessages(DataValidationLog, ValidationContext);
		DataValidationLog.Flush();
	}
	// ASSET VALIDATION END

	// Dont let other async compilation warnings be attributed incorrectly to the package that is loading.
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(AssetValidation_WaitAssetCompilation, AssetValidationChannel);
		WaitForAssetCompilationIfNecessary(InSettings.ValidationUsecase);
	}

	int32 PrevNumChecked	= OutResults.NumChecked;
	int32 PrevNumInvalid	= OutResults.NumInvalid;
	OutResults.NumRequested = AssetDataList.Num();

	const auto& UserSettings = UAssetValidationSettings::Get();
	// Now add to map or update as needed
	for (const FAssetData& AssetData : AssetDataList)
	{
		ensure(AssetData.IsValid());

		if (SlowTask.ShouldCancel())
		{
			// break the loop if task cancel was requested by user
			break;
		}
		
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
			if (UserSettings->bEnabledDetailedAssetLogging)
			{
				DataValidationLog.Info()
				->AddToken(FAssetDataToken::Create(AssetData))
				->AddToken(FTextToken::Create(LOCTEXT("ValidatingAsset", "Skipping asset, directory is excluded.")));
			}
			++OutResults.NumSkipped;
			continue;
		}

		const bool bLoadAsset = false;
		if (!InSettings.bLoadAssetsForValidation && !AssetData.FastGetAsset(bLoadAsset))
		{
			++OutResults.NumSkipped;
			continue;
		}

		// do not log detailed info for every asset unless asked to
		if (UserSettings->bEnabledDetailedAssetLogging)
		{
			DataValidationLog.Info()
			->AddToken(FAssetDataToken::Create(AssetData))
			->AddToken(FTextToken::Create(LOCTEXT("ValidatingAsset", "Validating asset")));
			UE_LOG(LogAssetValidation, Display, TEXT("Validating asset %s"), *AssetData.ToSoftObjectPath().ToString());
		}
		
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
		EDataValidationResult AssetResult = IsAssetValidWithContext(AssetData, ValidationContext);
// ASSET VALIDATION END
		
		// Don't add more messages to ValidationContext after this point because we will no longer add them to the message log
		UE::AssetValidation::AppendAssetValidationMessages(DataValidationLog, AssetData, ValidationContext);

		const bool bAnyWarnings = ValidationContext.GetNumWarnings() > 0;

		++OutResults.NumChecked;
		if (AssetResult == EDataValidationResult::Valid)
		{
			if (bAnyWarnings)
			{
				++OutResults.NumWarnings;
				DataValidationLog.Warning()
				->AddToken(FAssetDataToken::Create(AssetData))
				->AddToken(FTextToken::Create(LOCTEXT("ContainsWarningsResult", "contains valid data, but has warnings.")));
			}
			else
			{
				++OutResults.NumValid;
			}
		}
		else if (AssetResult == EDataValidationResult::Invalid)
		{
			++OutResults.NumInvalid;
			DataValidationLog.Error()
			->AddToken(FAssetDataToken::Create(AssetData))
			->AddToken(FTextToken::Create(LOCTEXT("InvalidDataResult", "contains invalid data.")));
		}
		else if (AssetResult == EDataValidationResult::NotValidated)
		{
			++OutResults.NumSkipped;
			if (InSettings.bShowIfNoFailures)
			{
				DataValidationLog.Info()
				->AddToken(FAssetDataToken::Create(AssetData))
				->AddToken(FTextToken::Create(LOCTEXT("NotValidatedDataResult", "has no data validation.")));
			}
		}
		
		if (InSettings.bCollectPerAssetDetails)
		{
			FValidateAssetsDetails& Details = OutResults.AssetsDetails.Emplace(AssetData.GetObjectPathString());
			Details.PackageName = AssetData.PackageName;
			Details.AssetName = AssetData.AssetName;
			Details.Result = AssetResult;
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

	// calculate and return validation result
	if (OutResults.NumInvalid > PrevNumInvalid)
	{
		return EDataValidationResult::Invalid;
	}
	if (OutResults.NumChecked > PrevNumChecked)
	{
		return EDataValidationResult::Valid;
	}

	return EDataValidationResult::NotValidated;
}

EDataValidationResult UAssetValidationSubsystem::ValidateChangelistsInternal(
	FMessageLog& 								DataValidationLog,
	TConstArrayView<UDataValidationChangelist*> Changelists,
	const FValidateAssetsSettings& Settings,
	FValidateAssetsResults& OutResults) const
{
	FScopedSlowTask SlowTask(Changelists.Num(), LOCTEXT("AssetValidation.ValidatingChangelistTask", "Validating Changelists"));
	SlowTask.Visibility = ESlowTaskVisibility::Invisible;
	SlowTask.MakeDialog(false);

	IAssetRegistry& AssetRegistry = IAssetRegistry::GetChecked();
	if (AssetRegistry.IsLoadingAssets())
	{
		UE_CLOG(FApp::IsUnattended(), LogAssetValidation, Fatal, TEXT("Unable to perform unattended content validation while asset registry scan is in progress. Callers just wait for asset registry scan to complete."));
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("AssetValidation.UnableToValidate_PendingAssetRegistry", "Unable to validate changelist while asset registry scan is in progress. Wait until asset discovery is complete."));
		return EDataValidationResult::NotValidated;
	}
	
	for (UDataValidationChangelist* CL : Changelists)
	{
		CL->AddToRoot();
	}
	
	ON_SCOPE_EXIT
	{
		for (UDataValidationChangelist* CL : Changelists)
		{
			CL->RemoveFromRoot();		
		}
	};

	EDataValidationResult Result = EDataValidationResult::NotValidated;
	for (UDataValidationChangelist* Changelist : Changelists)
	{
		FText ValidationMessage = FText::Format(LOCTEXT("ValidatingChangelistMessage", "Validating changelist {0}"), Changelist->Description);
		DataValidationLog.Info(ValidationMessage);
		SlowTask.EnterProgressFrame(1.0f, ValidationMessage);

		FDataValidationContext ValidationContext(false, Settings.ValidationUsecase, {}); // No associated objects for changelist
		EDataValidationResult AssetResult = IsObjectValidWithContext(Changelist, ValidationContext);
		Result &= AssetResult;

// ASSET VALIDATION BEGIN fix warnings/errors not being added when PerAssetDetails is enabled
		if (Settings.bCollectPerAssetDetails)
		{
			FValidateAssetsDetails& Details = OutResults.AssetsDetails.Emplace(Changelist->GetPathName());
			Details.AssetName = FName{Changelist->Description.ToString()};
			Details.Result = AssetResult;
			ValidationContext.SplitIssues(Details.ValidationWarnings, Details.ValidationErrors);
		}
// ASSET VALIDATION END

		UE::DataValidation::AddAssetValidationMessages(DataValidationLog, ValidationContext);
		DataValidationLog.Flush();
	}

	TSet<FAssetData> AssetsToValidate;	
	for (UDataValidationChangelist* Changelist : Changelists)
	{
		FDataValidationContext ValidationContext(false, Settings.ValidationUsecase, {}); // No associated objects for changelist
		GatherAssetsToValidateFromChangelist(Changelist, Settings, AssetsToValidate, ValidationContext);
		UE::DataValidation::AddAssetValidationMessages(DataValidationLog, ValidationContext);
		DataValidationLog.Flush();
	}

// ASSET VALIDATION BEGIN replace TSet with TArray
	// Filter out assets that we don't want to validate
	TArray<FAssetData> Assets{AssetsToValidate.Array()};
	{
		// clear assets that we shouldn't validate
		FDataValidationContext ValidationContext(false, Settings.ValidationUsecase, {});
		Assets.SetNum(Algo::RemoveIf(Assets, [this, &Settings, &ValidationContext](const FAssetData& AssetData)
		{
			if (!ShouldValidateAsset(AssetData, Settings, ValidationContext))
			{
				UE_LOG(LogAssetValidation, Verbose, TEXT("Excluding asset %s from validation"), *AssetData.GetSoftObjectPath().ToString());
				return true;
			}

			return false;
		}));
// ASSET VALIDATION END
		UE::DataValidation::AddAssetValidationMessages(DataValidationLog, ValidationContext);
		DataValidationLog.Flush();
	}

	// Validate assets from all changelists
	Result &= ValidateAssetsInternalResolver(DataValidationLog, Assets, Settings, OutResults);

	return Result;
}

EDataValidationResult UAssetValidationSubsystem::IsAssetValidWithContext(const FAssetData& AssetData, FDataValidationContext& InContext) const
{
	EDataValidationResult Result = EDataValidationResult::NotValidated;
	if (!AssetData.IsValid())
	{
		return Result;
	}

	if (ValidatedAssets.Contains(AssetData))
	{
		// asset has already been validated, skipping
		return Result;
	}
	
	if (!CurrentSettings.IsSet())
	{
		CurrentSettings = UAssetValidationSettings::Get()->DefaultSettings;
	}
	
	// explicitly increase validated assets count
	++CheckedAssetsCount; 
	
	const UObject* Asset = AssetData.FastGetAsset(false);
	if (Asset == nullptr && ShouldLoadAsset(AssetData))
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
		UE::AssetValidation::AppendAssetValidationMessages(InContext, AssetData, LogGatherer);
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

	MarkAssetDataValidated(AssetData, Result);
	return Result;
}

EDataValidationResult UAssetValidationSubsystem::IsActorValidWithContext(const FAssetData& AssetData, AActor* Actor, FDataValidationContext& InContext) const
{
	// Actor should already be loaded, it is either external actor or a usual actor
	check(AssetData.IsValid() && Actor != nullptr);
	
	EDataValidationResult Result = EDataValidationResult::NotValidated;
	if (ValidatedAssets.Contains(AssetData))
	{
		// asset has already been validated, skipping
		return Result;
	}
	
	if (!CurrentSettings.IsSet())
	{
		CurrentSettings = UAssetValidationSettings::Get()->DefaultSettings;
	}
	
	// explicitly increase validated assets count
	++CheckedAssetsCount;

	for (UAssetValidator* ActorValidator: ActorValidators)
	{
		ActorValidator->ResetValidationState();
		if (ActorValidator->K2_CanValidate(InContext.GetValidationUsecase()) && ActorValidator->CanValidateAsset_Implementation(AssetData, Actor, InContext))
		{
			// validate actor with asset data. Actor should already be loaded, it is either external actor or a usual actor
			Result &= ActorValidator->ValidateLoadedAsset(AssetData, Actor, InContext);
		}
	}

	MarkAssetDataValidated(AssetData, Result);
	return Result;
}

bool UAssetValidationSubsystem::ShouldShowCancelButton(int32 NumAssets, const FValidateAssetsSettings& InSettings) const
{
	return (InSettings.ValidationUsecase == EDataValidationUsecase::Manual || InSettings.ValidationUsecase == EDataValidationUsecase::Script)
			&& NumAssets > UAssetValidationSettings::Get()->NumAssetsToShowCancelButton;
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

void UAssetValidationSubsystem::MarkAssetDataValidated(const FAssetData& AssetData, EDataValidationResult Result) const
{
	ValidatedAssets.Add(AssetData);
	ValidationResults[static_cast<uint8>(Result)] += 1;
}

void UAssetValidationSubsystem::ResetValidationState() const
{
	const_cast<UAssetValidationSubsystem&>(*this).ResetValidationState();
}

void UAssetValidationSubsystem::ResetValidationState()
{
	CheckedAssetsCount = 0;
	LoadedPackageNames.Empty(32);
	ValidatedAssets.Empty(32);
	FMemory::Memzero(ValidationResults.GetData(), ValidationResults.Num() * sizeof(int32));

	CurrentSettings.Reset();
}

#undef LOCTEXT_NAMESPACE