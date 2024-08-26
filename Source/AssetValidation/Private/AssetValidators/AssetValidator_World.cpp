#include "AssetValidators/AssetValidator_World.h"

#include "EditorWorldUtils.h"
#include "Engine/LevelScriptActor.h"
#include "Engine/LevelScriptBlueprint.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/DataLayer/DataLayerManager.h"
#include "WorldPartition/LoaderAdapter/LoaderAdapterShape.h"

#include "AssetValidationDefines.h"
#include "AssetValidationStatics.h"
#include "AssetValidationSubsystem.h"
#include "LevelUtils.h"
#include "PackageTools.h"
#include "WorldPartition/ErrorHandling/WorldPartitionStreamingGenerationMapCheckErrorHandler.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

bool UAssetValidator_World::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	if (!Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext))
	{
		return false;
	}
	
	if (InAssetData.AssetClassPath != UWorld::StaticClass()->GetClassPathName())
	{
		return false;
	}
	
	if (InObject != nullptr && !InObject->IsA<UWorld>())
	{
		return false;
	}
	
	if (InContext.GetValidationUsecase() == EDataValidationUsecase::Save)
	{
		if (const UWorld* World = CastChecked<UWorld>(InObject, ECastCheckedType::NullAllowed))
		{
			if (World->IsPartitionedWorld() && InContext.GetAssociatedExternalObjects().Num() == 0)
			{
				// saving WP actors would result in this validator attempting to run with world asset
				// don't validate whole world on save, only external actors, wait until PreSubmit or Manual
				return false;
			}
			else if (EstimateWorldAssetCount(World) > ValidateOnSaveAssetCountThreshold)
			{
				return false;
			}
		}
	}

	return true;
}

int32 UAssetValidator_World::EstimateWorldAssetCount(const UWorld* World) const
{
	check(World);
	// rough estimation of a number of assets that are going to be validated as part of world validation
	// world, persistent level, level script blueprint, level script actor, world partition
	int32 WorldAssetCount = 5; 
	// for each streaming level, validate level, script blueprint and script actor
	WorldAssetCount += World->GetStreamingLevels().Num() * 3;
	// for each level, validate actors
	for (const ULevel* Level: World->GetLevels())
	{
		WorldAssetCount += Level->Actors.Num();
	}
	// sublevel worlds are not initialized so we have to handle persistent level separately
	if (!World->bIsWorldInitialized)
	{
		WorldAssetCount += World->PersistentLevel->Actors.Num();
	}

	return WorldAssetCount;
}


EDataValidationResult UAssetValidator_World::ValidateAsset_Implementation(const FAssetData& AssetData, FDataValidationContext& Context)
{
	check(AssetData.IsValid());

	if (Context.GetValidationUsecase() == EDataValidationUsecase::Save && Context.GetAssociatedExternalObjects().Num() > 0)
	{
		return ValidateExternalAssets(AssetData, Context);
	}
	
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
	check(InAsset && InAssetData.IsValid());

	if (InContext.GetValidationUsecase() == EDataValidationUsecase::Save && InContext.GetAssociatedExternalObjects().Num() > 0)
	{
		return ValidateExternalAssets(InAssetData, InContext);
	}

	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_ValidateWorld, AssetValidationChannel);

	// guard against recursive world validation. We're not safe 
	TGuardValue RecursiveGuard{bRecursiveGuard, true};

	UWorld* World = CastChecked<UWorld>(InAsset);
	
	TUniquePtr<FScopedEditorWorld> ScopedWorld{};
	if (!World->bIsWorldInitialized)
	{
		// ensure that world is not a sublevel.
		// Sublevel worlds in editor are not initialize, so we can't initialize/cleanup them either
		if (FLevelUtils::FindStreamingLevel(World->PersistentLevel) == nullptr)
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

	// notify number of assets that validate as a part of world asset validation
	Context.AddMessage(EMessageSeverity::Info,
		FText::Format(NSLOCTEXT("AssetValidation", "WorldValidation_AdditionalInfo", "Validating {0} assets as a part of {1}"),
		FText::FromString(FString::FromInt(EstimateWorldAssetCount(World))), FText::FromName(AssetData.AssetName)));
	
	EDataValidationResult Result = EDataValidationResult::Valid;
	// don't validate world settings explicitly, actor iterator will walk over it
	Result &= ValidateAssetInternal(*Subsystem, World->PersistentLevel, Context);
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
        	if (ULevel* Level = LevelStreaming->GetLoadedLevel())
        	{
        		Result &= ValidateAssetInternal(*Subsystem, Level, Context);
        		Result &= ValidateAssetInternal(*Subsystem, Level->LevelScriptBlueprint, Context);
        		Result &= ValidateAssetInternal(*Subsystem, Level->GetLevelScriptActor(), Context);
        	}
        }
	}

	// starting from 5.4 world calls IsDataValid on actor using FActorIterator, so we don't call it.
	// Otherwise we would be calling AActor::IsDataValid twice
	for (const ULevel* Level: World->GetLevels())
	{
		for (AActor* Actor: Level->Actors)
		{
			Result &= ValidateAssetInternal(*Subsystem, Actor, Context);
		}
	}

	// sublevel worlds are not initialized, so we can't use an actor iterator on them
	if (!World->bIsWorldInitialized)
	{
		for (AActor* Actor: World->PersistentLevel->Actors)
		{
			Result &= ValidateAssetInternal(*Subsystem, Actor, Context);
		}
	}

	// @todo: validate level instances?

	return Result;
}

EDataValidationResult UAssetValidator_World::ValidateAssetInternal(const UAssetValidationSubsystem& ValidationSubsystem, AActor* Actor, FDataValidationContext& Context)
{
	const FAssetData AssetData{Actor};
	LogValidatingAssetMessage(AssetData, Context);
	
	return ValidationSubsystem.IsActorValidWithContext(AssetData, Actor, Context);
}

EDataValidationResult UAssetValidator_World::ValidateAssetInternal(const UAssetValidationSubsystem& ValidationSubsystem, UObject* Asset, FDataValidationContext& Context)
{
	const FAssetData AssetData{Asset};
	LogValidatingAssetMessage(AssetData, Context);
	
	return ValidationSubsystem.IsAssetValidWithContext(AssetData, Context);
}

EDataValidationResult UAssetValidator_World::ValidateExternalAssets(const FAssetData& InAssetData, FDataValidationContext& Context)
{
	using namespace UE::AssetValidation;
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_ExternalAssets, AssetValidationChannel);
	
	const UAssetValidationSubsystem* Subsystem = UAssetValidationSubsystem::Get();

	const uint32 NumValidationErrors = Context.GetNumErrors();
	EDataValidationResult Result = EDataValidationResult::NotValidated;
	for (const FAssetData& ExternalAsset: Context.GetAssociatedExternalObjects())
	{
		// gather error and warning messages produced by loading an external asset
		FScopedLogMessageGatherer Gatherer{ExternalAsset, Context};
		if (UObject* Asset = ExternalAsset.GetAsset({ULevel::LoadAllExternalObjectsTag}))
		{
			TGuardValue AssetGuard{CurrentExternalAsset, Asset};

			LogValidatingAssetMessage(ExternalAsset, Context);
			if (AActor* Actor = Cast<AActor>(Asset))
			{
				Result &= Subsystem->IsActorValidWithContext(ExternalAsset, Actor, Context);
			}
			else
			{
				Result &= Subsystem->IsAssetValidWithContext(ExternalAsset, Context);
			}
		}
		else
		{
			Context.AddMessage(ExternalAsset, EMessageSeverity::Error, FText::Format(LOCTEXT("AssetLoadFailed", "Failed to load asset {0}"), FText::FromName(ExternalAsset.AssetName)));
			Result &= EDataValidationResult::Invalid;
		}
	}

	if (Result == EDataValidationResult::Invalid)
	{
		check(Context.GetNumErrors() > NumValidationErrors);
		const FText FailReason = FText::Format(LOCTEXT("AssetCheckFailed", "{0} is not valid. See AssetCheck log for more details"), FText::FromName(InAssetData.AssetName));
		Context.AddMessage(InAssetData, EMessageSeverity::Error, FailReason);

		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}

#undef LOCTEXT_NAMESPACE