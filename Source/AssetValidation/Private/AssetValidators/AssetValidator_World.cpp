#include "AssetValidators/AssetValidator_World.h"

#include "EditorValidatorSubsystem.h"
#include "EditorWorldUtils.h"
#include "Engine/LevelScriptActor.h"
#include "Engine/LevelScriptBlueprint.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/DataLayer/DataLayerManager.h"
#include "WorldPartition/LoaderAdapter/LoaderAdapterShape.h"

#include "AssetValidationModule.h"
#include "AssetValidationSubsystem.h"
#include "EngineUtils.h"
#include "PackageTools.h"

bool UAssetValidator_World::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	EDataValidationUsecase Usecase = InContext.GetValidationUsecase();
	if (Usecase == EDataValidationUsecase::Save)
	{
		// saving WP actors would result in this validator attempting to run with world asset
		// don't validate on save, wait until PreSubmit or Manual
		return false;
	}

	if (InObject != nullptr && !InObject->IsA<UWorld>())
	{
		return false;
	}
	return true;
}

EDataValidationResult UAssetValidator_World::ValidateAsset_Implementation(const FAssetData& AssetData, FDataValidationContext& Context)
{
	check(AssetData.IsValid());
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_WorldActors, AssetValidationChannel);
	
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
			Result &= ValidateLoadedAsset_Implementation(AssetData, World, Context);
		}
		// force unload package, otherwise engine may crash trying to re-load the same world
		UPackageTools::UnloadPackages({WorldPackage});
	}

	return Result;
}

EDataValidationResult UAssetValidator_World::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext)
{
	check(bRecursiveGuard == false);
	check(InAsset);

	// guard against recursive world validation. We're not safe 
	TGuardValue RecursiveGuard{bRecursiveGuard, true};

	UWorld* World = CastChecked<UWorld>(InAsset);
	
	TUniquePtr<FScopedEditorWorld> ScopedWorld{};
	if (!World->bIsWorldInitialized)
	{
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
	else if (bEnsureInactiveWorld)
	{
		// @todo: if world is already initialized, it either means it is already opened (active map in editor), or loaded via GetAsset call
		// second option sets WorldType == Inactive which means world is not properly initialized
		ensureAlwaysMsgf(World->WorldType != EWorldType::Inactive, TEXT("World %s skipped proper editor initialization, loaded as inactive"), *World->GetName());
	}
	
	const uint32 NumValidationErrors = InContext.GetNumErrors();
	const EDataValidationResult Result = ValidateWorld(World, InContext);
	if (Result == EDataValidationResult::Valid)
	{
		AssetPasses(World);
	}
	else
	{
		check(InContext.GetNumErrors() > NumValidationErrors);
		FText FailReason = FText::Format(NSLOCTEXT("AssetValidation", "WorldActors_Failed_AssetCheck", "{0} is not valid. See AssetCheck log for more details."), FText::FromString(World->GetName()));
		AssetFails(World, FailReason);
	}
	
	return Result;
}

EDataValidationResult UAssetValidator_World::ValidateWorld(UWorld* World, FDataValidationContext& Context)
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
	
	UAssetValidationSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetValidationSubsystem>();
	check(Subsystem);

	EDataValidationResult Result = EDataValidationResult::Valid;
	// don't validate world settings explicitly, actor iterator will walk over it
	Result &= ValidateAssetInternal(*Subsystem, World->PersistentLevel->LevelScriptBlueprint, Context);
	Result &= ValidateAssetInternal(*Subsystem, World->PersistentLevel->GetLevelScriptActor(), Context);
	
	if (!UWorld::IsPartitionedWorld(World))
	{
		// validate sublevel level blueprints
		for (const ULevelStreaming* LevelStreaming: World->GetStreamingLevels())
		{
			if (const ULevel* Level = LevelStreaming->GetLoadedLevel())
			{
				Result &= ValidateAssetInternal(*Subsystem, World->PersistentLevel->LevelScriptBlueprint, Context);
				Result &= ValidateAssetInternal(*Subsystem, Level->GetLevelScriptActor(), Context);
			}
		}
	}

#if !WITH_DATA_VALIDATION_UPDATE // starting from 5.4 world calls IsDataValid on actor using FActorIterator
	Result &= World->IsDataValid(Context);
#endif
	for (FActorIterator It{World, AActor::StaticClass(), EActorIteratorFlags::AllActors}; It; ++It)
	{
		AActor* Actor = *It;
		Result &= ValidateAssetInternal(*Subsystem, Actor, Context);
	}

	// @todo: validate level instances?

	return Result;
}

EDataValidationResult UAssetValidator_World::ValidateAssetInternal(UAssetValidationSubsystem& ValidationSubsystem, UObject* Asset, FDataValidationContext& Context)
{
#if WITH_DATA_VALIDATION_UPDATE // actor validation is fixed in 5.4
	FAssetData AssetData{Asset};
	LogValidatingAssetMessage(AssetData, Context);
	
	return ValidationSubsystem.IsAssetValidWithContext(AssetData, Context);
#else
	return ValidationSubsystem->IsStandaloneActorValid(Asset, Context);
#endif
}
