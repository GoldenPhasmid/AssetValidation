#include "AssetValidators/AssetValidator_WorldActors.h"

#include "EditorValidatorSubsystem.h"
#include "EditorWorldUtils.h"
#include "EngineUtils.h"
#include "Engine/LevelScriptActor.h"
#include "Engine/LevelScriptBlueprint.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/DataLayer/DataLayerManager.h"
#include "WorldPartition/LoaderAdapter/LoaderAdapterShape.h"

#include "AssetValidationModule.h"
#include "AssetValidationSubsystem.h"
#include "PackageTools.h"

bool UAssetValidator_WorldActors::CanValidate_Implementation(const EDataValidationUsecase InUsecase) const
{
	// save current use case to pass it for world actor validation
	const_cast<ThisClass*>(this)->CurrentUseCase = InUsecase;
#if 0
	return InUsecase != EDataValidationUsecase::Save && InUsecase != EDataValidationUsecase::PreSubmit;
#endif
	return true;
}

bool UAssetValidator_WorldActors::CanValidateAsset_Implementation(UObject* InAsset) const
{
	return Super::CanValidateAsset_Implementation(InAsset) && InAsset != nullptr && InAsset->IsA<UWorld>();
}

EDataValidationResult UAssetValidator_WorldActors::ValidateAsset(const FAssetData& AssetData, TArray<FText>& ValidationErrors)
{
	check(AssetData.IsValid());

#if 0
	const FString WorldPackageName = InAsset->GetPackage()->GetName();
	// @todo: this doesn't actually work, package does not get reset
	// currently the only way I see from the engine code is to iterate all package objects, 
	// remove GARBAGE_COLLECTION_KEEPFLAGS and call garbage collection. Which can take a while
	InAsset->GetPackage()->MarkAsUnloaded();
#endif

#if 0
    UWorld::WorldTypePreLoadMap.FindOrAdd(FName(WorldPackageName)) = EWorldType::Editor;


    UPackage* WorldPackage = CreatePackage(*FString{TEXT("TEMP_") + WorldPackageName});
    check(WorldPackage);

    WorldPackage->MarkAsUnloaded();

    WorldPackage->SetPackageFlags(PKG_ContainsMap);
    WorldPackage->SetLoadedPath(InAsset->GetPackage()->GetLoadedPath());
    WorldPackage->FullyLoad();
    
    UWorld::WorldTypePreLoadMap.Remove(FName(WorldPackageName));
#endif

	EDataValidationResult Result = EDataValidationResult::Valid;
	if (UPackage* WorldPackage = LoadWorldPackageForEditor(AssetData.PackageName.ToString(), EWorldType::Editor))
	{
		if (UWorld* World = UWorld::FindWorldInPackage(WorldPackage))
		{
			Result &= ValidateLoadedAsset(World, ValidationErrors);
		}
		// force unload package, otherwise engine may crash trying to re-load the same world
		UPackageTools::UnloadPackages({WorldPackage});
	}

	return Result;
}

EDataValidationResult UAssetValidator_WorldActors::ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors)
{
	check(bRecursiveGuard == false);
	check(InAsset);

	// guard against recursive world validation. We're not safe 
	TGuardValue{bRecursiveGuard, true};

	UWorld* World = CastChecked<UWorld>(InAsset);
	
	TUniquePtr<FScopedEditorWorld> ScopedWorld;
	if (!World->bIsWorldInitialized)
	{
		// @todo: figure out how to load a proper editor world in such cases
		UWorld::InitializationValues IVS;
		IVS.RequiresHitProxies(false);
		IVS.ShouldSimulatePhysics(false);
		IVS.EnableTraceCollision(false);
		IVS.CreateNavigation(false);
		IVS.CreateAISystem(false);
		IVS.AllowAudioPlayback(false);
		IVS.CreatePhysicsScene(true);

		ScopedWorld = MakeUnique<FScopedEditorWorld>(World, IVS, EWorldType::Editor);
	}

	const int32 NumValidationErrors = ValidationErrors.Num();
	const EDataValidationResult Result = ValidateWorld(World, ValidationErrors);
	if (Result == EDataValidationResult::Valid)
	{
		AssetPasses(World);
	}
	else
	{
		check(ValidationErrors.Num() > NumValidationErrors);
		FText ErrorMsg = FText::Format(NSLOCTEXT("AssetValidation", "WorldActors_Failed_AssetCheck", "{0} is not valid. See AssetCheck log for more details."), FText::FromString(World->GetName()));
		AssetFails(World, ErrorMsg, ValidationErrors);
	}
	
	return Result;
}

EDataValidationResult UAssetValidator_WorldActors::ValidateWorld(UWorld* World, TArray<FText>& ValidationErrors)
{
	TUniquePtr<FLoaderAdapterShape> AllActors;
	
	if (UWorld::IsPartitionedWorld(World))
	{
		if (UWorldPartition* WorldPartition = World->GetWorldPartition())
		{
			UDataLayerManager* DataLayerManager = WorldPartition->GetDataLayerManager();
			DataLayerManager->ForEachDataLayerInstance([](UDataLayerInstance* DataLayer)
			{
				// load all data layers
				DataLayer->SetIsLoadedInEditor(true, false);
				if (!IsRunningCommandlet() || IsAllowCommandletRendering())
				{
					DataLayer->SetIsInitiallyVisible(true);
				}

				return true;
			});

			// notify editor cells: don't ask me how this works
			FDataLayersEditorBroadcast::StaticOnActorDataLayersEditorLoadingStateChanged(false);
		}

		AllActors = MakeUnique<FLoaderAdapterShape>(World, FBox{FVector{-HALF_WORLD_MAX}, FVector{HALF_WORLD_MAX}}, TEXT("Loaded Region"));
        AllActors->Load();
	}
	else
	{
		// @todo: load all sublevels for non-partitioned worlds
	}
	
	UAssetValidationSubsystem* ValidationSubsystem = GEditor->GetEditorSubsystem<UAssetValidationSubsystem>();
	check(ValidationSubsystem);

	EDataValidationResult Result = EDataValidationResult::Valid;
	// don't validate world settings explicitly, actor iterator will walk over it
	TArray<FText> ValidationWarnings;
	Result &= ValidationSubsystem->IsObjectValid(World->PersistentLevel->LevelScriptBlueprint, ValidationErrors, ValidationWarnings, CurrentUseCase);
	Result &= ValidationSubsystem->IsStandaloneActorValid(World->PersistentLevel->GetLevelScriptActor(), ValidationErrors, ValidationWarnings, CurrentUseCase);

	if (!UWorld::IsPartitionedWorld(World))
	{
		// validate sublevel level blueprints
		for (const ULevelStreaming* LevelStreaming: World->GetStreamingLevels())
		{
			if (const ULevel* Level = LevelStreaming->GetLoadedLevel())
			{
				Result &= ValidationSubsystem->IsObjectValid(World->PersistentLevel->LevelScriptBlueprint, ValidationErrors, ValidationWarnings, CurrentUseCase);
				Result &= ValidationSubsystem->IsStandaloneActorValid(Level->GetLevelScriptActor(), ValidationErrors, ValidationWarnings, CurrentUseCase);
			}
		}
	}
	
	for (FActorIterator It{World, AActor::StaticClass(), EActorIteratorFlags::AllActors}; It; ++It)
	{
		AActor* Actor = *It;
		ValidationSubsystem->IsStandaloneActorValid(Actor, ValidationErrors, ValidationWarnings, CurrentUseCase);
	}

	// @todo: validate level instances?

	return Result;
}
