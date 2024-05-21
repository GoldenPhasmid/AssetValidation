#include "AssetValidationStatics.h"

#include "AssetValidationDefines.h"
#include "AssetValidationModule.h"
#include "DataValidationModule.h"
#include "EditorValidatorHelpers.h"
#include "EditorValidatorSubsystem.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "ShaderCompiler.h"
#include "SourceControlHelpers.h"
#include "SourceControlProxy.h"
#include "AssetRegistry/AssetDataToken.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Logging/MessageLog.h"
#include "Misc/ScopedSlowTask.h"
#include "Misc/UObjectToken.h"
#include "Settings/ProjectPackagingSettings.h"

#include "StudioTelemetry.h"

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
		
		FMessageLog DataValidationLog(UE::DataValidation::MessageLogName);
		DataValidationLog.NewPage(LOCTEXT("ValidatePackages", "Validate Packages"));
		
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

#if 0
		FLogMessageGatherer Gatherer; // @todo: it is an open question whether it works when multiple data validation logs are opened
#endif
		FValidateAssetsSettings Settings = InSettings;
		ValidatorSubsystem->ValidateAssetsWithSettings(Assets, Settings, OutResults);
	}

	bool IsCppFile(const FString& Filename)
	{
		return Filename.EndsWith(TEXT(".h")) || Filename.EndsWith(TEXT(".cpp")) || Filename.EndsWith(TEXT(".hpp"));
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
#if 0
		for (const FDataValidationContext::FIssue& Issue : ValidationContext.GetIssues())
		{
			if (Issue.TokenizedMessage.IsValid())
			{
				MessageLog.AddMessage(Issue.TokenizedMessage.ToSharedRef());
			}
			else
			{
				
				AppendAssetValidationMessages(MessageLog, AssetData, Issue.Severity, { Issue.Message });
			}
		}
#endif
#if 0
		TArray<FText> Warnings, Errors;
		Warnings.Reserve(ValidationContext.GetNumWarnings());
		Errors.Reserve(ValidationContext.GetNumErrors());
		
		ValidationContext.SplitIssues(Warnings, Errors);

		AppendAssetValidationMessages(MessageLog, AssetData, EMessageSeverity::Error, Errors);
		AppendAssetValidationMessages(MessageLog, AssetData, EMessageSeverity::Warning, Warnings);
#endif
	}

#if 0
	void AppendAssetValidationMessages(FMessageLog& MessageLog, const FAssetData& AssetData, EMessageSeverity::Type Severity, TConstArrayView<FText> Messages)
	{
		// use object name if working with external package
		const UObject* Asset = AssetData.FastGetAsset(false);
		const bool bExternalPackage = Asset && Asset->IsPackageExternal();
		const FString PackageName = bExternalPackage ? Asset->GetName() : AssetData.PackageName.ToString();
		
		for (const FText& Msg: Messages)
		{
			MessageLog.AddMessage(AssetData, Severity, Msg);

			MessageLog.AddMessage(CreateAssetMessage(Msg, AssetData, Severity));
		}
	}
#endif

#if 0
	TSharedRef<FTokenizedMessage> CreateAssetMessage(const FText& Message, const FAssetData& AssetData, EMessageSeverity::Type Severity)
	{
		// @todo: optimize for loops
		const UObject* Asset = AssetData.FastGetAsset(false);
		const bool bExternalPackage = Asset && Asset->IsPackageExternal();
		const FString PackageName = bExternalPackage ? Asset->GetName() : AssetData.PackageName.ToString();
		
		if (bExternalPackage)
		{
			return FTokenizedMessage::Create(Severity)
			// asset log prefix
			->AddToken(FTextToken::Create(FText::FromString(TEXT("[AssetLog]")))) 
			// asset token
			->AddToken(FAssetDataToken::Create(Asset)) 
			// colon after object token to separate error message
			->AddToken(FTextToken::Create(FText::FromString(TEXT(":"))))
			// actual error message
			->AddToken(FTextToken::Create(Message));
		}
		else
		{
			const FString FormattedPath		= FAssetMsg::FormatPathForAssetLog(*PackageName);
			const FString AssetLogString	= FAssetMsg::GetAssetLogString(*PackageName, Message.ToString());
				
			FString BeforeAsset{}, AfterAsset{};
			TSharedRef<FTokenizedMessage> TokenizedMessage = FTokenizedMessage::Create(Severity);
			if (AssetLogString.Split(FormattedPath, &BeforeAsset, &AfterAsset))
			{
				if (!BeforeAsset.IsEmpty())
				{
					TokenizedMessage->AddToken(FTextToken::Create(FText::FromString(BeforeAsset)));
				}
				
				TokenizedMessage->AddToken(FAssetDataToken::Create);
					
				// add asset name to AfterAsset for cases like default object validation and asset validation that are not yet on disk
				FString AssetNameString = AssetData.AssetName.ToString();
				AssetNameString.RemoveFromStart(TEXT("Default__"));
				AfterAsset.InsertAt(0, AssetNameString);
				
				if (!AfterAsset.IsEmpty())
				{
					TokenizedMessage->AddToken(FTextToken::Create(FText::FromString(AfterAsset)));
				}
			}
			else
			{
				TokenizedMessage->AddToken(FTextToken::Create(FText::FromString(AssetLogString)));
			}

			return TokenizedMessage;
		}
	}
#endif

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
#if 0
			ValidationContext.AddMessage(CreateAssetMessage(Msg, AssetData, Severity));
#endif
			
		}
	}

	void AppendAssetValidationMessages(FDataValidationContext& ValidationContext, const FAssetData& AssetData, EMessageSeverity::Type Severity, TConstArrayView<FString> Messages)
	{
		for (const FString& Msg: Messages)
		{
			ValidationContext.AddMessage(AssetData, Severity, FText::FromString(Msg));
#if 0
			ValidationContext.AddMessage(CreateAssetMessage(FText::FromString(Msg), AssetData, Severity));
#endif
		}
	}
	
	void ValidatePackages(TConstArrayView<FString> ModifiedPackages, TConstArrayView<FString> DeletedPackages, const FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(UE::AssetValidation::ValidatePackages);
		
		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

		FMessageLog ValidationLog("AssetCheck");
		ValidationLog.NewPage(LOCTEXT("ValidatePackages", "Validate Packages"));

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
#if 0
		FLogMessageGatherer Gatherer; // @todo: it is an open question whether it works when multiple data validation logs are opened
#endif
	
		FValidateAssetsSettings Settings = InSettings;
		Settings.bCaptureAssetLoadLogs = false;		// do not capture asset load logs
		Settings.bSkipExcludedDirectories = true;	// skip excluded directories
		
		UEditorValidatorSubsystem* ValidatorSubsystem = GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>();
		FValidateAssetsResults Results;
		ValidatorSubsystem->ValidateAssetsWithSettings(AssetsToValidate, Settings, Results);
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

		if (PackageName.StartsWith(TEXT("/Game/Developers/")))
		{
			return false;
		}

		return true;
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
} // UE::AssetValidation

#undef LOCTEXT_NAMESPACE
