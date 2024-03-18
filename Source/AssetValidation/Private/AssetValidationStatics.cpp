#include "AssetValidationStatics.h"

#include "AssetValidationModule.h"
#include "DataValidationModule.h"
#include "EditorValidatorSubsystem.h"
#include "FileHelpers.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "ShaderCompiler.h"
#include "SourceControlHelpers.h"
#include "SourceControlOperations.h"
#include "StudioAnalytics.h"
#include "WorldPartitionSourceControlValidator.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Logging/MessageLog.h"
#include "Misc/ScopedSlowTask.h"
#include "WorldPartition/ActorDescContainerCollection.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionActorDescUtils.h"
#include "WorldPartition/DataLayer/WorldDataLayers.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

void AssetValidationStatics::ValidateSourceControl(bool bInteractive, EDataValidationUsecase ValidationUsecase, TArray<FString>& OutWarnings, TArray<FString>& OutErrors)
{
	FStudioAnalytics::RecordEvent(TEXT("ValidateChangelist"));

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

	if (GShaderCompilingManager)
	{
		// finish all shader compilation to avoid weird errors
		FScopedSlowTask SlowTask(0.f, LOCTEXT("CompileShaders", "Compiling shaders..."));
		SlowTask.MakeDialog();

		GShaderCompilingManager->FinishAllCompilation();
	}

	// Step 1: Gather Source Control Files and Packages
	TArray<FSourceControlStateRef> FileStates;
	TArray<FString> ChangedPackages;
	
	if (ISourceControlModule::Get().IsEnabled())
	{
		ISourceControlProvider& SCCProvider = ISourceControlModule::Get().GetProvider();
		SCCProvider.Execute(ISourceControlOperation::Create<FUpdateStatus>(), EConcurrency::Synchronous);
		
		FileStates = SCCProvider.GetCachedStateByPredicate([](const FSourceControlStateRef& State)
		{
			return State->IsCheckedOut() || State->IsAdded() || State->IsDeleted();
		});
	}

	ChangedPackages.Reserve(FileStates.Num());
	for (const FSourceControlStateRef& File: FileStates)
	{
		FString Filename = File->GetFilename();
		if (FPackageName::IsPackageFilename(Filename))
		{
			FString PackageName;
			if (FPackageName::TryConvertFilenameToLongPackageName(Filename, PackageName))
			{
				// @todo: handle deleted packages separately
				ChangedPackages.Add(PackageName);
			}
		}
		// @todo: validation for header changes
	}

	// Step 2: Validate Dirty Files
	ValidateDirtyFiles(FileStates, OutWarnings, OutErrors);
	
	// Step 3: Validate World Partition Actors
	{
		FScopedSlowTask SlowTask(0.f, LOCTEXT("CheckWorldPartition", "Checking World Partition..."));
		SlowTask.MakeDialog();
		
		ValidateWorldPartitionActors(FileStates, OutWarnings, OutErrors);
	}
	
	// Step 4: Validate Source Control Packages
	{
		FScopedSlowTask SlowTask(0.f, LOCTEXT("CheckContentTask", "Checking content..."));
		SlowTask.MakeDialog();

		ValidatePackages(ChangedPackages, ValidationUsecase, OutWarnings, OutErrors);
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

void AssetValidationStatics::ValidateWorldPartitionActors(const TArray<FSourceControlStateRef>& FileStates, TArray<FString>& OutWarnings, TArray<FString>& OutErrors)
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
			if (const UClass* ActorClass = AssetValidationStatics::GetAssetNativeClass(AssetData))
			{
				FTopLevelAssetPath WorldPath = AssetValidationStatics::GetAssetWorld(AssetData);
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
					if (const UClass* DataLayerReferencerClass = AssetValidationStatics::GetAssetNativeClass(DataLayerReferencer))
					{
						FTopLevelAssetPath WorldPath = AssetValidationStatics::GetAssetWorld(DataLayerReferencer);
						WorldToActors.FindOrAdd(WorldPath).Add(DataLayerReferencerClass);
					}
				}
					
				DataLayerAssets.Add(AssetData.PackageName.ToString());
			}
			else if (AssetClass->IsChildOf<UWorld>())
			{
				FTopLevelAssetPath WorldPath = AssetValidationStatics::GetAssetWorld(AssetData);
				WorldToActors.Add(WorldPath);
			}
		}
	}

	// For Each world 
	for (auto& [WorldAssetPath, AssetData]: WorldToActors)
	{
		// Find/Load the ActorDescContainer
		UWorld* World = FindObject<UWorld>(nullptr, *WorldAssetPath.ToString(), true);

		FActorDescContainerCollection ActorContainers;
		for (const FAssetData& Asset: AssetData)
		{
			const FString ActorPackagePath = Asset.PackagePath.ToString();

			//@todo: ActorPackagePath can be in content bundle.
			RegisterActorContainer(World, WorldAssetPath.GetPackageName(), ActorContainers);
		}
		
		FWorldPartitionValidatorParams ValidationParams;
		ValidationParams.RelevantMap = WorldAssetPath;
		ValidationParams.bHasWorldDataLayers = bHasWorldDataLayers;
		ValidationParams.RelevantDataLayerAssets = DataLayerAssets;

		// Build a set of Relevant Actor Guids to scope error messages to what's contained in the CL 
		for (const FAssetData& Asset : AssetData)
		{
			// Get the FWorldPartitionActor			
			const FWorldPartitionActorDesc* ActorDesc = ActorContainers.GetActorDescByName(Asset.AssetName.ToString());

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

		ActorContainers.ForEachActorDescContainer([&Params](const UActorDescContainer* ActorDescContainer)
		{
			for (FActorDescList::TConstIterator<> ActorDescIt(ActorDescContainer); ActorDescIt; ++ActorDescIt)
			{
				check(!Params.ActorGuidsToContainerMap.Contains(ActorDescIt->GetGuid()));
				Params.ActorGuidsToContainerMap.Add(ActorDescIt->GetGuid(), ActorDescContainer);
			}
		});

		ActorContainers.ForEachActorDescContainer([&Params](const UActorDescContainer* ActorDescContainer)
		{
			Params.ActorDescContainer = ActorDescContainer;
			UWorldPartition::CheckForErrors(Params);
		});
	}
}

void AssetValidationStatics::ValidateDirtyFiles(const TArray<FSourceControlStateRef>& FileStates, TArray<FString>& OutWarnings, TArray<FString>& OutErrors)
{
	TArray<UPackage*> DirtyPackages;
	FEditorFileUtils::GetDirtyPackages(DirtyPackages, FEditorFileUtils::FShouldIgnorePackage::Default);

	TSet<FString> DirtyPackagePaths;
	Algo::Transform(DirtyPackages, DirtyPackagePaths, AssetValidationStatics::GetPackagePath);
	
	for (const FSourceControlStateRef& FileState : FileStates)
	{
		const FString Filename = FileState->GetFilename();
		if (DirtyPackagePaths.Contains(Filename))
		{
			OutErrors.Add(FString::Printf(TEXT("File %s contains unsaved modifications, save it"), *Filename));
		}
	}
}

FString AssetValidationStatics::ValidateEmptyPackage(const FString& PackageName)
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

FString AssetValidationStatics::GetPackagePath(const UPackage* Package)
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

FTopLevelAssetPath AssetValidationStatics::GetAssetWorld(const FAssetData& AssetData)
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

void AssetValidationStatics::RegisterActorContainer(UWorld* World, FName ContainerPackageName, FActorDescContainerCollection& RegisteredContainers)
{
	if (RegisteredContainers.Contains(ContainerPackageName))
	{
		return;
	}

	UActorDescContainer* ActorDescContainer = nullptr;
	if (World != nullptr)
	{
		// World is Loaded reuse the ActorDescContainer of the Content Bundle
		ActorDescContainer = World->GetWorldPartition()->FindContainer(ContainerPackageName);
	}

	// Even if world is valid, its world partition is not necessarily initialized
	if (!ActorDescContainer)
	{
		// Find in memory failed, load the ActorDescContainer
		ActorDescContainer = NewObject<UActorDescContainer>();
		ActorDescContainer->Initialize({ nullptr, ContainerPackageName });
	}

	RegisteredContainers.AddContainer(ActorDescContainer);
}

UClass* AssetValidationStatics::GetAssetNativeClass(const FAssetData& AssetData)
{
	// Check that the asset is an actor
	if (FWorldPartitionActorDescUtils::IsValidActorDescriptorFromAssetData(AssetData))
	{
		return FWorldPartitionActorDescUtils::GetActorNativeClassFromAssetData(AssetData);
	}

	return nullptr;
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
			UPackage* Package = Asset.GetPackage();
			if (Package->HasAnyPackageFlags(PKG_ContainsMap | PKG_ContainsMapData))
			{
				continue;
			}
			
			if (!UWorld::IsWorldOrExternalActorPackage(Package))
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

bool AssetValidationStatics::ValidateProjectSettings(EDataValidationUsecase ValidationUsecase, TArray<FString>& OutWarnings, TArray<FString>& OutErrors)
{
	TArray<UClass*> AllClasses;
	GetDerivedClasses(UObject::StaticClass(), AllClasses, true);

	TArray<FAssetData> Assets;
	Assets.Reserve(AllClasses.Num());

	// gather default config classes and classes that derive from UDeveloperSettings
	for (const UClass* Class: AllClasses)
	{
		if (Class->IsChildOf(UDeveloperSettings::StaticClass()))
		{
			FAssetData AssetData{Class->GetDefaultObject()};
			Assets.Add(AssetData);
		}
		else if (Class->HasAnyClassFlags(EClassFlags::CLASS_DefaultConfig) && Class->ClassConfigName != TEXT("Input"))
		{
			FAssetData AssetData{Class->GetDefaultObject()};
			Assets.Add(AssetData);
		}
	}
	
	const UEditorValidatorSubsystem* ValidatorSubsystem = GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>();

	FValidateAssetsSettings Settings;
	Settings.bSkipExcludedDirectories = true;
	Settings.bShowIfNoFailures = true;
	Settings.ValidationUsecase = ValidationUsecase;
	
	FLogMessageGatherer Gatherer;
	
	FValidateAssetsResults Results;
	ValidatorSubsystem->ValidateAssetsWithSettings(Assets, Settings, Results);

	OutErrors.Append(Gatherer.GetErrors());
	OutWarnings.Append(Gatherer.GetWarnings());

	return Results.NumInvalid > 0;
}



#undef LOCTEXT_NAMESPACE
