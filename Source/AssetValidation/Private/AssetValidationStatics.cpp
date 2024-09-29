#include "AssetValidationStatics.h"

#include "AssetValidationDefines.h"
#include "AssetValidationModule.h"
#include "AssetValidationSettings.h"
#include "DataValidationModule.h"
#include "EditorValidatorHelpers.h"
#include "EditorValidatorSubsystem.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "ShaderCompiler.h"
#include "SourceControlProxy.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Logging/MessageLog.h"
#include "Misc/ScopedSlowTask.h"
#include "Settings/ProjectPackagingSettings.h"

#include "StudioTelemetry.h"
#include "AssetValidators/AssetValidator.h"
#include "IMessageLogListing.h"
#include "AssetRegistry/AssetDataToken.h"
#include "Misc/UObjectToken.h"
#include "Presentation/MessageLogListingViewModel.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

namespace UE::AssetValidation
{
	
FScopedLogMessageGatherer::FScopedLogMessageGatherer(const FAssetData& InAssetData, FDataValidationContext& InContext)
	: FOutputDevice()
    , AssetData(InAssetData)
    , Context(InContext)
{
    GLog->AddOutputDevice(this);
}

FScopedLogMessageGatherer::FScopedLogMessageGatherer(const FAssetData& InAssetData, FDataValidationContext& InContext, TFunction<FString(const FString&)> InLogConverter)
	: FScopedLogMessageGatherer(InAssetData, InContext)
{
	LogConverter = InLogConverter;
}

FScopedLogMessageGatherer::~FScopedLogMessageGatherer()
{
    std::scoped_lock Lock{CriticalSection};
    GLog->RemoveOutputDevice(this);
}

void FScopedLogMessageGatherer::Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category)
{
    std::scoped_lock Lock{CriticalSection};

	FString Message{V};
	if (LogConverter)
	{
		Message = LogConverter(Message);
	}
	
    const FText MessageText = FText::FromString(Message);
    switch (Verbosity)
    {
    case ELogVerbosity::Warning:
    	Context.AddMessage(AssetData, EMessageSeverity::Warning, MessageText);
    	break;
    case ELogVerbosity::Error:
    	Context.AddMessage(AssetData, EMessageSeverity::Error, MessageText);
    	break;
    default:
    	break;
    }
}
	
} // UE::AssetValidation

namespace UE::AssetValidation
{
	int32 ValidateCheckedOutAssets(bool bInteractive, const FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(UE::AssetValidation::ValidateCheckedOutAssets);
		
		if (FStudioTelemetry::IsAvailable())
		{
			FStudioTelemetry::Get().RecordEvent(TEXT("ValidateCheckedOutAssets"));
		}
		
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
			return 0;
		}

		if (GShaderCompilingManager && GShaderCompilingManager->IsCompiling())
		{
			// finish all shader compilation to avoid weird errors
			FScopedSlowTask SlowTask(0.f, LOCTEXT("CompileShaders", "Finishing shader compilation..."));
			SlowTask.MakeDialog();

			GShaderCompilingManager->FinishAllCompilation();
		}

		// Step 1: Gather Source Control Files
		TArray<FSourceControlStateRef> FileStates;

		TSharedRef<ISourceControlProxy> SCProxy = IAssetValidationModule::Get().GetSourceControlProxy();
		SCProxy->GetOpenedFiles(FileStates);

		// Step 2: Group Source Control Files by Type
		TArray<FString> ModifiedPackages, DeletedPackages, ModifiedFiles, DeletedFiles;
		for (const FSourceControlStateRef& FileState: FileStates)
		{
			FString Filename = FileState->GetFilename();
			// group source controlled files by the type
			if (FPackageName::IsPackageFilename(Filename))
			{
				// file state is a package, either added, modified or deleted
				FString PackageName;
				if (FPackageName::TryConvertFilenameToLongPackageName(Filename, PackageName))
				{
					if (FileState->IsDeleted())
					{
						DeletedPackages.Add(PackageName);
					}
					else
					{
						ModifiedPackages.Add(PackageName);
					}
				}
			}
			else if (IsCppFile(Filename))
			{
				if (FileState->IsDeleted())
				{
					DeletedFiles.Add(Filename);
				}
				else
				{
					ModifiedFiles.Add(Filename);	
				}
				// @todo: handle source files
			}
		}
		
		// Step 5: Validate Source Control Modified Packages
		{
			FScopedSlowTask SlowTask(0.f, LOCTEXT("CheckContentTask", "Checking content..."));
			SlowTask.MakeDialog();
			
			ValidatePackages(ModifiedPackages, DeletedPackages, InSettings, OutResults);
		}

		// Step 6: Validate Project Settings
		{
			FScopedSlowTask SlowTask(0.f, LOCTEXT("CheckProjectSetings", "Checking project settings..."));
			SlowTask.MakeDialog();
			
			ValidateProjectSettings(InSettings, OutResults);
		}
		
		if (bInteractive)
		{
			const bool bAnyFailures = OutResults.NumInvalid > 0 || OutResults.NumWarnings > 0;
			if (!bAnyFailures)
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("SourceControl_Success", "Content validation completed for {0} files."),
					FText::AsNumber(FileStates.Num())));
			}
			else if (OutResults.NumWarnings > 0)
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("SourceControl_PartialFailure", "Content validation fount {0} warnings. Check output log for more details."),
					FText::AsNumber(OutResults.NumWarnings)));
			}
			else
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("SourceControl_Failure", "Content validation found {0} errors, {1} warnings. Check output log for more details."),
					FText::AsNumber(OutResults.NumInvalid), FText::AsNumber(OutResults.NumWarnings)));
			}
		}
		else
		{
			UE_LOG(LogAssetValidation, Display, TEXT("Content validation for source controlled files finished. Found %d warnings, %d errors for %d assets."),
				OutResults.NumWarnings, OutResults.NumInvalid, OutResults.NumRequested);
		}

		return OutResults.NumInvalid + OutResults.NumWarnings;
	}
	
	void ValidateProjectSettings(const FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(UE::AssetValidation::ValidateProjectSettings);
		
		TArray<UClass*> AllClasses;
		GetDerivedClasses(UObject::StaticClass(), AllClasses, true);

		TArray<FAssetData> Assets;
		Assets.Reserve(AllClasses.Num());

		// gather classes that have DefaultConfig class specifier and classes that derive from UDeveloperSettings
		for (const UClass* Class: AllClasses)
		{
			if (Class->IsChildOf<UDeveloperSettings>())
			{
				// derives from UDeveloperSettings
				FAssetData AssetData{Class->GetDefaultObject()};
				Assets.Add(AssetData);
			}
			else if (Class->HasAnyClassFlags(EClassFlags::CLASS_DefaultConfig) && Class->ClassConfigName != TEXT("Input"))
			{
				// class is default config class
				FAssetData AssetData{Class->GetDefaultObject()};
				Assets.Add(AssetData);
			}
		}
	
		const UEditorValidatorSubsystem* ValidatorSubsystem = GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>();
		check(ValidatorSubsystem);
		
		FValidateAssetsSettings Settings = InSettings;
		Settings.MessageLogPageTitle = LOCTEXT("ValidateProjectSettings", "Validating Project Settings...");
		ValidatorSubsystem->ValidateAssetsWithSettings(Assets, Settings, OutResults);
	}

	bool IsCppFile(const FString& Filename)
	{
		return Filename.EndsWith(TEXT(".h")) || Filename.EndsWith(TEXT(".cpp")) || Filename.EndsWith(TEXT(".hpp"));
	}

	bool GetAssetSizeBytes(IAssetRegistry& AssetRegistry, const FAssetData& AssetData, float& OutMemorySize, float& OutDiskSize)
	{
		if (!AssetData.IsValid())
		{
			return false;
		}
		
		if (TOptional<FAssetPackageData> FoundData = AssetRegistry.GetAssetPackageDataCopy(AssetData.PackageName); FoundData.IsSet())
		{
			OutDiskSize = FoundData->DiskSize;
		}
		else
		{
			OutDiskSize = 0.f;
		}

		if (UObject* Asset = AssetData.GetAsset())
		{
			OutMemorySize = Asset->GetResourceSizeBytes(EResourceSizeMode::EstimatedTotal);
		}
		else
		{
			OutMemorySize = 0.f;
		}
		
		return true;
	}

	UClass* GetCppBaseClass(UClass* InClass)
	{
		while (InClass != nullptr && InClass->GetClass()->IsChildOf<UBlueprintGeneratedClass>())
		{
			InClass = InClass->GetSuperClass();
		}

		return InClass;
	}

	void ClearLogMessages(FMessageLog& MessageLog)
	{
		if (auto OnGetLog = MessageLog.OnGetLog(); OnGetLog.IsBound())
		{
			TSharedRef<IMessageLog> Log = OnGetLog.Execute(UE::DataValidation::MessageLogName);
			
			TSharedRef<FMessageLogListingViewModel> LogListing = StaticCastSharedRef<FMessageLogListingViewModel>(Log);
			LogListing->ClearMessages();
		}
	}

	void AppendAssetValidationMessages(FMessageLog& MessageLog, FDataValidationContext& ValidationContext)
	{
		for (const FDataValidationContext::FIssue& Issue : ValidationContext.GetIssues())
		{
			if (Issue.TokenizedMessage.IsValid())
			{
				MessageLog.AddMessage(Issue.TokenizedMessage.ToSharedRef());
			}
			else
			{
				MessageLog.Message(Issue.Severity, Issue.Message);
			}
		}
	}

	void AppendAssetValidationMessages(FMessageLog& MessageLog, const FAssetData& AssetData, FDataValidationContext& ValidationContext)
	{
		UE::DataValidation::AddAssetValidationMessages(AssetData, MessageLog, ValidationContext);
	}

	void AppendAssetValidationMessages(FDataValidationContext& ValidationContext, const FAssetData& AssetData, UE::DataValidation::FScopedLogMessageGatherer& Gatherer)
	{
		TArray<FString> Warnings{}, Errors{};
		Gatherer.Stop(Warnings, Errors);

		AppendAssetValidationMessages(ValidationContext, AssetData, EMessageSeverity::Error, Errors);
		AppendAssetValidationMessages(ValidationContext, AssetData, EMessageSeverity::Warning, Warnings);
	}
	

	void AppendAssetValidationMessages(FDataValidationContext& ValidationContext, const FAssetData& AssetData, EMessageSeverity::Type Severity, TConstArrayView<FText> Messages)
	{
		for (const FText& Msg: Messages)
		{
			ValidationContext.AddMessage(AssetData, Severity, Msg);
		}
	}

	void AppendAssetValidationMessages(FDataValidationContext& ValidationContext, const FAssetData& AssetData, EMessageSeverity::Type Severity, TConstArrayView<FString> Messages)
	{
		for (const FString& Msg: Messages)
		{
			ValidationContext.AddMessage(AssetData, Severity, FText::FromString(Msg));
		}
	}
	
	void ValidatePackages(TConstArrayView<FString> ModifiedPackages, TConstArrayView<FString> DeletedPackages, const FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(UE::AssetValidation::ValidatePackages);
		
		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

		TArray<FString> AllPackages{ModifiedPackages};
		for (const FString& DeletedPackage: DeletedPackages)
		{
			// append referencer packages found from deleted packages
			TArray<FName> Referencers;
			AssetRegistry.GetReferencers(FName(DeletedPackage), Referencers, AssetRegistry::EDependencyCategory::Package, AssetRegistry::EDependencyQuery::Hard);
			for (const FName& Referencer: Referencers)
			{
				const FString ReferenceStr{Referencer.ToString()};
				if (!DeletedPackages.Contains(ReferenceStr) && ShouldValidatePackage(ReferenceStr))
				{
					UE_LOG(LogAssetValidation, Verbose, TEXT("%s: Deleted package %s references package %s, added to validation"), *FString(__FUNCTION__), *DeletedPackage, *ReferenceStr);
					AllPackages.Add(ReferenceStr);
				}
			}
		}

		FMessageLog ValidationLog{UE::DataValidation::MessageLogName};
		
		TArray<FAssetData> AssetsToValidate;
		for (const FString& PackageName: AllPackages)
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
				if (FString WarningMessage = ValidateEmptyPackage(PackageName); !WarningMessage.IsEmpty())
				{
					UE_LOG(LogAssetValidation, Warning, TEXT("%s"), *WarningMessage);
                    ValidationLog.Warning(FText::FromString(WarningMessage));
				}
			}
			else
			{
				AssetsToValidate.Append(MoveTemp(Assets));
			}
		}
		ValidationLog.Flush();

		FValidateAssetsSettings Settings = InSettings;
		Settings.bCaptureAssetLoadLogs = false;		// do not capture asset load logs
		Settings.bSkipExcludedDirectories = true;	// skip excluded directories
		Settings.MessageLogPageTitle = LOCTEXT("ValidatePackages", "Validating Packages...");
		
		UEditorValidatorSubsystem* ValidatorSubsystem = GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>();
		ValidatorSubsystem->ValidateAssetsWithSettings(AssetsToValidate, Settings, OutResults);
	}

	bool ShouldValidatePackage(const FString& PackageName)
	{
		const UProjectPackagingSettings* PackagingSettings = GetDefault<UProjectPackagingSettings>();

		for (const FDirectoryPath& Directory: PackagingSettings->DirectoriesToNeverCook)
		{
			const FString& Folder = Directory.Path;
			if (PackageName.StartsWith(Folder))
			{
				return false;
			}
		}
		
		for (const FDirectoryPath& Directory: UAssetValidationSettings::Get()->ExcludedDirectories)
		{
			const FString& Folder = Directory.Path;
			if (PackageName.StartsWith(Folder))
			{
				return false;
			}
		}
		
		return true;
	}

	void SetValidatorEnabled(UEditorValidatorBase* Validator, bool bEnabled)
	{
		if (UAssetValidator* AssetValidator = Cast<UAssetValidator>(Validator))
		{
			// do it the correct way
			AssetValidator->SetEnabled(bEnabled);
		}
		else
		{
			// do it the unreal way, because everything we need is always protected or private without a getter/setter
			const FBoolProperty* Property = CastFieldChecked<FBoolProperty>(Validator->GetClass()->FindPropertyByName(TEXT("bIsEnabled")));
			Property->SetPropertyValue(Validator, bEnabled);
		}
	}

	FString ValidateEmptyPackage(const FString& PackageName)
	{
		FString Message{};
		if (FPackageName::DoesPackageExist(PackageName))
		{
			Message = FString::Printf(TEXT("Found no assets in package %s"), *PackageName);
		}
		else if (ISourceControlModule::Get().IsEnabled())
		{
			ISourceControlProvider& SCCProvider = ISourceControlModule::Get().GetProvider();
			FString PackageFilename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
			auto FileState = SCCProvider.GetState(PackageFilename, EStateCacheUsage::ForceUpdate);

			if (FileState->IsAdded())
			{
				Message = FString::Printf(TEXT("Package '%s' is missing from disk. It is marked for add in perforce but missing from your hard drive."), *PackageName);
			}

			if (FileState->IsCheckedOut())
			{
				Message = FString::Printf(TEXT("Package '%s' is missing from disk. It is checked out in perforce but missing from your hard drive."), *PackageName);
			}

			if (Message.IsEmpty())
			{
				Message = FString::Printf(TEXT("Package '%s' is missing from disk."), *PackageName);
			}
		}
		check(!Message.IsEmpty());

		return Message;
	}
	
	bool IsWorldOrWorldExternalPackage(UPackage* Package)
	{
		return UWorld::IsWorldOrWorldExternalPackage(Package);
	}

	bool IsWorldAsset(const FAssetData& AssetData)
	{
		return AssetData.AssetClassPath == UWorld::StaticClass()->GetClassPathName();
	}

	bool IsExternalAsset(const FString& PackagePath)
	{
		return PackagePath.Contains(FPackagePath::GetExternalActorsFolderName()) || PackagePath.Contains(FPackagePath::GetExternalObjectsFolderName());
	}

	bool IsExternalAsset(const FAssetData& AssetData)
	{
		return IsExternalAsset(AssetData.PackagePath.ToString());
	}

namespace Private
{
	template <>
	TSharedRef<FTokenizedMessage> AddToken<FText>(const TSharedRef<FTokenizedMessage>& Message, const FText& Text)
	{
		Message->AddText(Text);
		return Message;
	}

	template <>
	TSharedRef<FTokenizedMessage> AddToken<FString>(const TSharedRef<FTokenizedMessage>& Message, const FString& Str)
	{
		Message->AddText(FText::FromString(Str));
		return Message;
	}

	template <>
	TSharedRef<FTokenizedMessage> AddToken<FName>(const TSharedRef<FTokenizedMessage>& Message, const FName& Name)
	{
		Message->AddText(FText::FromName(Name));
		return Message;
	}

	template <>
	TSharedRef<FTokenizedMessage> AddToken<TSharedRef<IMessageToken>>(const TSharedRef<FTokenizedMessage>& Message, const TSharedRef<IMessageToken>& Token)
	{
		Message->AddToken(Token);
		return Message;
	}

	template <>
	TSharedRef<FTokenizedMessage> AddToken<FAssetData>(const TSharedRef<FTokenizedMessage>& Message, const FAssetData& AssetData)
	{
		Message->AddToken(FAssetDataToken::Create(AssetData));
		return Message;
	}

	template <>
	TSharedRef<FTokenizedMessage> AddToken<UObject>(const TSharedRef<FTokenizedMessage>& Message, const UObject* Object)
	{
		Message->AddToken(FUObjectToken::Create(Object));
		return Message;
	}

} // Private
} // UE::AssetValidation

#undef LOCTEXT_NAMESPACE
