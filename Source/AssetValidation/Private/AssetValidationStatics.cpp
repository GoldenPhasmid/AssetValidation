#include "AssetValidationStatics.h"

#include "AssetValidationDefines.h"
#include "AssetValidationModule.h"
#include "DataValidationModule.h"
#include "EditorValidatorSubsystem.h"
#include "FileHelpers.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "ShaderCompiler.h"
#include "SourceControlHelpers.h"
#include "SourceControlProxy.h"
#include "StudioAnalytics.h"
#include "AssetRegistry/AssetDataToken.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Logging/MessageLog.h"
#include "Misc/ScopedSlowTask.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionActorDescUtils.h"
#include "WorldPartition/DataLayer/WorldDataLayers.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

namespace UE::AssetValidation
{
	void ValidateCheckedOutAssets(bool bInteractive, EDataValidationUsecase ValidationUsecase, TArray<FString>& OutWarnings, TArray<FString>& OutErrors)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(UE::AssetValidation::ValidateCheckedOutAssets);

		FStudioAnalytics::RecordEvent(TEXT("ValidateCheckedOutAssets"));
		
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
		TArray ChangedPackages{TArray<FString>{}, static_cast<int32>(FileStates.Num() * 0.5)};
		TArray DeletedPackages{TArray<FString>{}, static_cast<int32>(FileStates.Num() * 0.5)};
		TArray SourceFiles{TArray<FString>{}, static_cast<int32>(FileStates.Num() * 0.5)};
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
						ChangedPackages.Add(PackageName);
					}
				}
			}
			else if (IsCppFile(Filename))
			{
				// file state 
				SourceFiles.Add(Filename); // @todo: handle source files
			}
		}

		// Step 2: Validate Dirty Files
		ValidateDirtyFiles(FileStates, OutWarnings, OutErrors);
	
		// Step 3: Validate World Partition Actors
		{
			FScopedSlowTask SlowTask(0.f, LOCTEXT("CheckWorldPartition", "Checking World Partition..."));
			SlowTask.MakeDialog();
		
			ValidateWorldPartitionActors(FileStates, OutWarnings, OutErrors);
		}
	
		// Step 4: Validate Source Control Modified Packages
		{
			FScopedSlowTask SlowTask(0.f, LOCTEXT("CheckContentTask", "Checking content..."));
			SlowTask.MakeDialog();

			ValidateModifiedPackages(ChangedPackages, ValidationUsecase, OutWarnings, OutErrors);
		}

		// Step 5: Validate Project Settings
		{
			FScopedSlowTask SlowTask(0.f, LOCTEXT("CheckProjectSetings", "Checking project settings..."));
			SlowTask.MakeDialog();
		
			ValidateProjectSettings(ValidationUsecase, OutWarnings, OutErrors);
		}
	
		if (bInteractive)
		{
			const bool bAnyMessages = OutErrors.Num() > 0  || OutWarnings.Num() > 0;
			if (!bAnyMessages)
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::Format(LOCTEXT("SourceControl_Success", "Content validation completed for {0} files."),
					FText::AsNumber(FileStates.Num())));
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
		else
		{
			UE_LOG(LogAssetValidation, Display, TEXT("Content validation for source controlled files finished. Found %d warnings, %d errors for %d files."),
				OutWarnings.Num(), OutErrors.Num(), FileStates.Num());
		}
	}

	void ValidateWorldPartitionActors(const TArray<FSourceControlStateRef>& FileStates, TArray<FString>& OutWarnings, TArray<FString>& OutErrors)
	{
		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	
		TSet<FString> DataLayerAssets;
		bool bHasWorldDataLayers = true;
	
		// Figure out which world(s) these actors are in and split the files per world
		TMap<FTopLevelAssetPath, TSet<FAssetData>> WorldToActors;
		for (const FSourceControlStateRef& File: FileStates)
		{
			// Skip deleted files since we're not validating references in this validator 
			if (File->IsDeleted())
			{
				continue;
			}

			FString PackageName;
			if (!FPackageName::TryConvertFilenameToLongPackageName(File->GetFilename(), PackageName))
			{
				OutErrors.Add(FString::Printf(TEXT("Cannot convert %s file name to package name"), *File->GetFilename()));
			}

			TArray<FAssetData> PackageAssets;
			USourceControlHelpers::GetAssetDataFromPackage(PackageName, PackageAssets);

			for (FAssetData& AssetData : PackageAssets)
			{
				if (const UClass* ActorClass = GetAssetNativeClass(AssetData))
				{
					FTopLevelAssetPath WorldPath = GetAssetWorld(AssetData);
					if (WorldPath.IsNull())
					{
						continue;
					}
					check(WorldPath.IsValid());
				
					WorldToActors.FindOrAdd(WorldPath).Add(ActorClass);

					if (ActorClass->IsChildOf<AWorldDataLayers>())
					{
						bHasWorldDataLayers = true;
						// @todo: World Data Layers
					}
					continue;
				}
			
				UClass* AssetClass = AssetData.GetClass();
				if (AssetClass == nullptr)
				{
					// @todo: handle assets with blueprint generated base classes
					continue;
				}
			
				if (AssetClass->IsChildOf<UDataLayerAsset>())
				{
					// gather all packages that this data layer references
					TArray<FName> ReferencerNames;
					AssetRegistry.GetReferencers(AssetData.PackageName, ReferencerNames, UE::AssetRegistry::EDependencyCategory::All);
					
					FARFilter Filter; 
					Filter.bIncludeOnlyOnDiskAssets = true;
					Filter.PackageNames = MoveTemp(ReferencerNames);
					Filter.ClassPaths.Add(AWorldDataLayers::StaticClass()->GetClassPathName());

					// gather all assets that this data layer references
					TArray<FAssetData> DataLayerReferencers;
					AssetRegistry.GetAssets(Filter, DataLayerReferencers);
					
					for (const FAssetData& DataLayerReferencer : DataLayerReferencers)
					{
						// add data layer referencers to WorldToActors map
						if (const UClass* DataLayerReferencerClass = GetAssetNativeClass(DataLayerReferencer))
						{
							FTopLevelAssetPath WorldPath = GetAssetWorld(DataLayerReferencer);
							WorldToActors.FindOrAdd(WorldPath).Add(DataLayerReferencerClass);
						}
					}
					
					DataLayerAssets.Add(AssetData.PackageName.ToString());
				}
				else if (AssetClass->IsChildOf<UWorld>())
				{
					FTopLevelAssetPath WorldPath = GetAssetWorld(AssetData);
					WorldToActors.Add(WorldPath);
				}
			}
		}

		// @todo: update to 5.4 asap
#if 0
		// For Each world
		for (auto& [WorldAssetPath, AssetData]: WorldToActors)
		{
			// Find/Load the ActorDescContainer
			UWorld* World = FindObject<UWorld>(nullptr, *WorldAssetPath.ToString(), true);

			FActorDescContainerCollection ContainerToValidate;
			for (const FAssetData& Asset: AssetData)
			{
				const FString ActorPackagePath = Asset.PackagePath.ToString();

				//@todo: ActorPackagePath can be in content bundle.
				RegisterActorContainer(World, WorldAssetPath.GetPackageName(), ContainerToValidate);
			}
		
			FWorldPartitionValidatorParams ValidationParams;
			ValidationParams.RelevantMap = WorldAssetPath;
			ValidationParams.bHasWorldDataLayers = bHasWorldDataLayers;
			ValidationParams.RelevantDataLayerAssets = DataLayerAssets;

			// Build a set of Relevant Actor Guids to scope error messages to what's contained in the CL 
			for (const FAssetData& Asset : AssetData)
			{
				// Get the FWorldPartitionActor			
				const FWorldPartitionActorDesc* ActorDesc = ContainerToValidate.GetActorDescByName(Asset.AssetName);

				if (ActorDesc != nullptr)
				{
					ValidationParams.RelevantActorGuids.Add(ActorDesc->GetGuid());
				}
			}
		
			// initialize World Partition Validator with current map params
			TSharedPtr<FWorldPartitionSourceControlValidator> Validator = MakeShared<FWorldPartitionSourceControlValidator>();
			Validator->Initialize(MoveTemp(ValidationParams));

			// Invoke static WorldPartition Validation from the ActorDescContainer
			UWorldPartition::FCheckForErrorsParams Params;
			Params.ErrorHandler = Validator.Get();
			Params.bEnableStreaming = !ULevel::GetIsStreamingDisabledFromPackage(WorldAssetPath.GetPackageName());

			ContainerToValidate.ForEachActorDescContainer([&Params](const UActorDescContainer* ActorDescContainer)
			{
				for (FActorDescList::TConstIterator<> ActorDescIt(ActorDescContainer); ActorDescIt; ++ActorDescIt)
				{
					check(!Params.ActorGuidsToContainerMap.Contains(ActorDescIt->GetGuid()));
					Params.ActorGuidsToContainerMap.Add(ActorDescIt->GetGuid(), ActorDescContainer);
				}
			});

			Params.ActorDescContainerCollection = &ContainerToValidate;
			UWorldPartition::CheckForErrors(Params);
		}
#endif
	}
	
	void ValidateDirtyFiles(const TArray<FSourceControlStateRef>& FileStates, TArray<FString>& OutWarnings, TArray<FString>& OutErrors)
	{
		TArray<UPackage*> DirtyPackages;
		FEditorFileUtils::GetDirtyPackages(DirtyPackages, FEditorFileUtils::FShouldIgnorePackage::Default);

		TSet<FString> DirtyPackagePaths;
		Algo::Transform(DirtyPackages, DirtyPackagePaths, GetPackagePath);
	
		for (const FSourceControlStateRef& FileState : FileStates)
		{
			const FString Filename = FileState->GetFilename();
			if (DirtyPackagePaths.Contains(Filename))
			{
				OutErrors.Add(FString::Printf(TEXT("File %s contains unsaved modifications, save it"), *Filename));
			}
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

	FString GetPackagePath(const UPackage* Package)
	{
		if (Package == nullptr)
		{
			return TEXT("");
		}

		const FString LocalFullPath(Package->GetLoadedPath().GetLocalFullPath());
	
		if (LocalFullPath.IsEmpty())
		{
			return TEXT("");
		}

		return FPaths::ConvertRelativePathToFull(LocalFullPath);
	}

	FTopLevelAssetPath GetAssetWorld(const FAssetData& AssetData)
	{
		// Check that the asset is an actor
		if (FWorldPartitionActorDescUtils::IsValidActorDescriptorFromAssetData(AssetData))
		{
			// WorldPartition actors are all in OFPA mode so they're external
			// Extract the MapName from the ObjectPath (<PathToPackage>.<MapName>:<Level>.<ActorName>)

			FSoftObjectPath ActorPath = AssetData.GetSoftObjectPath();
			FTopLevelAssetPath MapAssetName = ActorPath.GetAssetPath();

			if (ULevel::GetIsLevelPartitionedFromPackage(ActorPath.GetLongPackageFName()))
			{
				return MapAssetName;
			}
		}

		return FTopLevelAssetPath{};
	}

	bool IsCppFile(const FString& Filename)
	{
		return Filename.EndsWith(TEXT(".h")) || Filename.EndsWith(TEXT(".cpp")) || Filename.EndsWith(TEXT(".hpp"));
	}

	void AppendValidationMessages(FDataValidationContext& ValidationContext, const FAssetData& AssetData, FLogMessageGatherer& Gatherer)
	{
		TArray<FString> Warnings{}, Errors{};
		Gatherer.Release(Warnings, Errors);

		AppendValidationMessages(ValidationContext, AssetData, EMessageSeverity::Warning, Warnings);
		AppendValidationMessages(ValidationContext, AssetData, EMessageSeverity::Error, Errors);
	}
	
	void AppendValidationMessages(FDataValidationContext& ValidationContext, const FAssetData& AssetData, EMessageSeverity::Type Severity, TConstArrayView<FString> Messages)
	{
		if (Messages.Num())
		{
			TStringBuilder<2048> Buffer;
			Buffer.Join(Messages, LINE_TERMINATOR);

			ValidationContext.AddMessage(Severity)
			->AddToken(FAssetDataToken::Create(AssetData))
			->AddText(LOCTEXT("DataValidation.LoadWarnings", "Warnings loading asset {0}"), FText::FromStringView(Buffer.ToView()));
		}
	}

	// @todo: update to 5.4 asap
#if 0
	void RegisterActorContainer(UWorld* World, FName ContainerPackageName, FActorDescContainerCollection& RegisteredContainers)
	{
		if (RegisteredContainers.Contains(ContainerPackageName))
		{
			return;
		}

		TObjectPtr<UActorDescContainerInstance> ActorDescContainer = nullptr;
		if (World != nullptr)
		{
			// World is Loaded reuse the ActorDescContainer of the Content Bundle
			ActorDescContainer = World->GetWorldPartition()->FindContainer(ContainerPackageName);
		}

		// Even if world is valid, its world partition is not necessarily initialized
		if (!ActorDescContainer)
		{
			// Find in memory failed, load the ActorDescContainer
			ActorDescContainer = NewObject<UActorDescContainerInstance>();
			ActorDescContainer->Initialize(UActorDescContainerInstance::FInitializeParams{ContainerPackageName});
		}

		RegisteredContainers.AddContainer(ActorDescContainer);
	}
#endif

	UClass* GetAssetNativeClass(const FAssetData& AssetData)
	{
		// Check that the asset is an actor
		if (FWorldPartitionActorDescUtils::IsValidActorDescriptorFromAssetData(AssetData))
		{
			return FWorldPartitionActorDescUtils::GetActorNativeClassFromAssetData(AssetData);
		}

		return nullptr;
	}

	void ValidateModifiedPackages(const TArray<FString>& PackagesToValidate, EDataValidationUsecase Usecase, TArray<FString>& OutWarnings, TArray<FString>& OutErrors)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE(UE::AssetValidation::ValidatePackages);
		
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
				UPackage* Package = Asset.GetPackage();
				if (Package->HasAnyPackageFlags(PKG_ContainsMap | PKG_ContainsMapData))
				{
					continue;
				}
			
				if (!IsWorldOrWorldExternalPackage(Package))
				{
					continue;
				}
			
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

	void ValidateProjectSettings(EDataValidationUsecase ValidationUsecase, TArray<FString>& OutWarnings, TArray<FString>& OutErrors)
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

		FValidateAssetsSettings Settings;
		Settings.bSkipExcludedDirectories = true;
		Settings.bShowIfNoFailures = false;
		Settings.ValidationUsecase = ValidationUsecase;
		
		FLogMessageGatherer Gatherer;

		FValidateAssetsResults Results;
		ValidatorSubsystem->ValidateAssetsWithSettings(Assets, Settings, Results);

		OutErrors.Append(Gatherer.GetErrors());
		OutWarnings.Append(Gatherer.GetWarnings());
	}

	bool IsWorldOrWorldExternalPackage(UPackage* Package)
	{
#if WITH_DATA_VALIDATION_UPDATE
		return UWorld::IsWorldOrWorldExternalPackage(Package);
#else
		return UWorld::IsWorldOrExternalActorPackage(Package);
#endif
	}
	
} // UE::AssetValidation

#undef LOCTEXT_NAMESPACE
