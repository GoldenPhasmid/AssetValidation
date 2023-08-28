#include "AssetValidationStatics.h"

#include "AssetValidation.h"
#include "DataValidationModule.h"
#include "EditorValidatorSubsystem.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "ShaderCompiler.h"
#include "SourceControlHelpers.h"
#include "SourceControlOperations.h"
#include "StudioAnalytics.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Logging/MessageLog.h"
#include "Misc/ScopedSlowTask.h"
#include "Settings/ProjectPackagingSettings.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

void AssetValidationStatics::ValidateChangelistContent(bool bInteractive, EDataValidationUsecase ValidationUsecase, TArray<FString>& OutWarnings, TArray<FString>& OutErrors)
{
	FStudioAnalytics::RecordEvent(TEXT("ValidateContent"));

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	if (AssetRegistryModule.Get().IsLoadingAssets())
	{
		if (bInteractive)
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("DiscoveringAssets", "Still discovering assets. Try again once complete."));
		}
		else
		{
			UE_LOG(LogAssetValidation, Display, TEXT("Cannot validate content while still loading assets."));
		}
		return;
	}

	TArray<FString> ChangedPackages;
	
	if (ISourceControlModule::Get().IsEnabled())
	{
		ISourceControlProvider& SCCProvider = ISourceControlModule::Get().GetProvider();

		auto UpdateFiles = ISourceControlOperation::Create<FUpdateStatus>();
		UpdateFiles->SetGetOpenedOnly(true);
		SCCProvider.Execute(UpdateFiles, EConcurrency::Synchronous);
		
		TArray<FSourceControlStateRef> Files = SCCProvider.GetCachedStateByPredicate([](const FSourceControlStateRef& State)
		{
			return State->IsCheckedOut() || State->IsAdded() || State->IsDeleted();
		});

		ChangedPackages.Reserve(Files.Num());
		for (const FSourceControlStateRef& File: Files)
		{
			FString Filename = File->GetFilename();
			if (FPackageName::IsPackageFilename(Filename))
			{
				FString PackageName;
				if (FPackageName::TryConvertFilenameToLongPackageName(Filename, PackageName))
				{
					ChangedPackages.Add(PackageName);
				}
			}
		}
	}

	if (GShaderCompilingManager)
	{
		FScopedSlowTask SlowTask(0.f, LOCTEXT("CompileShaders", "Compiling shaders..."));
		SlowTask.MakeDialog();

		GShaderCompilingManager->FinishAllCompilation();
	}
	
	{
		FScopedSlowTask SlowTask(0.f, LOCTEXT("CheckContentTask", "Checking content..."));
		SlowTask.MakeDialog();

		ValidatePackages(ChangedPackages, ValidationUsecase, OutWarnings, OutErrors);
	}

	ValidateProjectSettings(ValidationUsecase, OutWarnings, OutErrors);

	if (bInteractive)
	{
		const bool bAnyMessages = OutErrors.Num() > 0  || OutWarnings.Num() > 0;
		if (!bAnyMessages)
		{
			FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("SourceControl_Success", "Content validation completed."));
		}
		else if (OutErrors.Num() == 0)
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("SourceControl_PartialFailure", "Content validation fount {0} warnings. Check output log for more details."),
				FText::AsNumber(OutWarnings.Num())));
		}
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("SourceControl_Failure", "Content validation found {0} errors, {1} warnings. Check output log for more details."),
				FText::AsNumber(OutErrors.Num()), FText::AsNumber(OutWarnings.Num())));
		}
	}
}

FString AssetValidationStatics::ValidateEmptyPackage(const FString& PackageName)
{
	FString WarningMessage{};
	if (FPackageName::DoesPackageExist(PackageName))
	{
		WarningMessage = FString::Printf(TEXT("Found no assets in package %s"), *PackageName);
	}
	else if (ISourceControlModule::Get().IsEnabled())
	{
		ISourceControlProvider& SCCProvider = ISourceControlModule::Get().GetProvider();
		FString PackageFilename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
		auto FileState = SCCProvider.GetState(PackageFilename, EStateCacheUsage::ForceUpdate);

		if (FileState->IsAdded())
		{
			WarningMessage = FString::Printf(TEXT("Package '%s' is missing from disk. It is marked for add in perforce but missing from your hard drive."), *PackageName);
		}

		if (FileState->IsCheckedOut())
		{
			WarningMessage = FString::Printf(TEXT("Package '%s' is missing from disk. It is checked out in perforce but missing from your hard drive."), *PackageName);
		}

		if (WarningMessage.IsEmpty())
		{
			WarningMessage = FString::Printf(TEXT("Package '%s' is missing from disk."), *PackageName);
		}
	}
	check(!WarningMessage.IsEmpty());

	return WarningMessage;
}

void AssetValidationStatics::ValidatePackages(const TArray<FString>& PackagesToValidate, EDataValidationUsecase Usecase, TArray<FString>& OutWarnings, TArray<FString>& OutErrors)
{
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	FMessageLog ValidationLog("AssetCheck");
	ValidationLog.NewPage(LOCTEXT("ValidatePackages", "Validate Packages"));
	
	TArray<FAssetData> AssetsToValidate;
	for (const FString& PackageName: PackagesToValidate)
	{
		if (!FPackageName::IsValidLongPackageName(PackageName))
		{
			UE_LOG(LogAssetValidation, Warning, TEXT("Invalid package long name %s"), *PackageName);
			continue;
		}
		
		TArray<FAssetData> Assets;
		AssetRegistry.GetAssetsByPackageName(FName(PackageName), Assets, true);

		if (Assets.IsEmpty())
		{
			FString WarningMessage = ValidateEmptyPackage(PackageName);
				
			UE_LOG(LogAssetValidation, Warning, TEXT("%s"), *WarningMessage);
			OutWarnings.Add(WarningMessage);
			ValidationLog.Warning(FText::FromString(WarningMessage));
		}
		else
		{
			AssetsToValidate.Append(MoveTemp(Assets));
		}
	}

	// Preload all assets to check, so load warnings can be handled separately from validation warnings
	for (const FAssetData& Asset: AssetsToValidate)
	{
		if (!Asset.IsAssetLoaded())
		{
			UE_LOG(LogAssetValidation, Display, TEXT("Loading %s"), *Asset.GetObjectPathString());
			
			FLogMessageGatherer Gatherer;
			(void)Asset.GetAsset();

			for (const FString& Warning: Gatherer.GetWarnings())
			{
				UE_LOG(LogAssetValidation, Warning, TEXT("%s"), *Warning);
			}
			for (const FString& Error: Gatherer.GetErrors())
			{
				UE_LOG(LogAssetValidation, Error, TEXT("%s"), *Error);
			}

			OutWarnings.Append(Gatherer.GetWarnings());
			OutErrors.Append(Gatherer.GetErrors());
		}
	}

	FLogMessageGatherer Gatherer;
	
	FValidateAssetsSettings Settings;
	Settings.bSkipExcludedDirectories = true;
	Settings.bShowIfNoFailures = true;
	Settings.ValidationUsecase = Usecase;
	FValidateAssetsResults Results;
	
	UEditorValidatorSubsystem* ValidatorSubsystem = GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>();
	const int32 Failures = ValidatorSubsystem->ValidateAssetsWithSettings(AssetsToValidate, Settings, Results);

	if (Failures > 0)
	{
		OutWarnings.Append(Gatherer.GetWarnings());
		OutErrors.Append(Gatherer.GetErrors());
	}
}

bool AssetValidationStatics::ValidateProjectSettings(EDataValidationUsecase ValidationUsecase, TArray<FString>& OutWarnings, TArray<FString>& OutErrors)
{
	TArray<FText> Warnings, Errors;
	const UEditorValidatorSubsystem* ValidatorSubsystem = GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>();
	EDataValidationResult Result = ValidatorSubsystem->IsObjectValid(GetMutableDefault<UProjectPackagingSettings>(), Errors, Warnings, ValidationUsecase);

	auto Trans = [](const FText& Text) { return Text.ToString(); };
	Algo::Transform(Errors, OutErrors, Trans);
	Algo::Transform(Warnings, OutWarnings, Trans);

	return Result != EDataValidationResult::Invalid;
}



#undef LOCTEXT_NAMESPACE
