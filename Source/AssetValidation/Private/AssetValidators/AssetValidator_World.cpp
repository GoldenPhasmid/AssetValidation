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
#include "WorldPartition/ErrorHandling/WorldPartitionStreamingGenerationMapCheckErrorHandler.h"

bool UAssetValidator_World::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	EDataValidationUsecase Usecase = InContext.GetValidationUsecase();
	if (InContext.GetAssociatedExternalObjects().Num() > 0 && Usecase == EDataValidationUsecase::Save)
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

	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_ValidateWorld, AssetValidationChannel);

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
	const EDataValidationResult Result = ValidateWorld(InAssetData, World, InContext);

	if (Result == EDataValidationResult::Invalid)
	{
		check(InContext.GetNumErrors() > NumValidationErrors);
		const FText FailReason = FText::Format(NSLOCTEXT("AssetValidation", "AssetCheckFailed", "{0} is not valid. See AssetCheck log for more details."), FText::FromString(World->GetName()));
		InContext.AddMessage(InAssetData, EMessageSeverity::Error, FailReason);
	}
	
	return Result;
}

EDataValidationResult UAssetValidator_World::ValidateWorld(const FAssetData& AssetData, UWorld* World, FDataValidationContext& Context)
{
	TUniquePtr<FLoaderAdapterShape> AllActors;
	
	if (UWorld::IsPartitionedWorld(World))
	{
		const UWorldPartition* WorldPartition = World->GetWorldPartition();
		// load all data layers
		UDataLayerManager* DataLayerManager = WorldPartition->GetDataLayerManager();
		DataLayerManager->ForEachDataLayerInstance([](UDataLayerInstance* DataLayer)
		{
				
			DataLayer->SetIsLoadedInEditor(true, false);
			if (!IsRunningCommandlet() || IsAllowCommandletRendering())
			{
				DataLayer->SetIsInitiallyVisible(true);
			}

			return true;
		});

		// notify editor cells: don't ask me how this works
		FDataLayersEditorBroadcast::StaticOnActorDataLayersEditorLoadingStateChanged(false);

		// load all world partition actors
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
	
	if (const UWorldPartition* WorldPartition = World->GetWorldPartition())
	{
		FStreamingGenerationMapCheckErrorHandler ErrorHandler;
		WorldPartition->CheckForErrors(&ErrorHandler);
	}
	else
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

	// starting from 5.4 world calls IsDataValid on actor using FActorIterator, so we don't call it.
	// Otherwise we would be calling AActor::IsDataValid twice
	
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
	FAssetData AssetData{Asset};
	LogValidatingAssetMessage(AssetData, Context);
	
	return ValidationSubsystem.IsAssetValidWithContext(AssetData, Context);
}
