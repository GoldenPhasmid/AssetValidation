#include "WorldPartitionSourceControlValidator.h"

#include "WorldPartition/DataLayer/DataLayerInstanceWithAsset.h"

#define LOCTEXT_NAMESPACE "WorldPartitionChangelistValidation"

void FWorldPartitionSourceControlValidator::Initialize(FWorldPartitionValidatorParams&& InParams)
{
	Params = InParams;
}

TArray<FText> FWorldPartitionSourceControlValidator::GetErrors() const
{
	return Errors;
}

bool FWorldPartitionSourceControlValidator::RelevantToSourceControl(const FWorldPartitionActorDescView& ActorDescView)
{
	if (Params.RelevantActorGuids.Find(ActorDescView.GetGuid()))
	{
		return true;
	}

	if (!Params.RelevantMap.IsNull())
	{
		FSoftObjectPath ActorPath = ActorDescView.GetActorSoftPath();
		return ActorPath.GetAssetPath() == Params.RelevantMap;
	}

	return false;
}

bool FWorldPartitionSourceControlValidator::RelevantToSourceControl(const UDataLayerInstance* DataLayerInstance)
{
	const UDataLayerInstanceWithAsset* DataLayerWithAsset = Cast<UDataLayerInstanceWithAsset>(DataLayerInstance);
	return DataLayerWithAsset != nullptr && DataLayerWithAsset->GetAsset() != nullptr && Params.RelevantDataLayerAssets.Contains(DataLayerWithAsset->GetAsset()->GetPathName());
}

void FWorldPartitionSourceControlValidator::OnInvalidRuntimeGrid(const FWorldPartitionActorDescView& ActorDescView, FName GridName)
{
	if (RelevantToSourceControl(ActorDescView))
	{
		FText CurrentError = FText::Format(LOCTEXT("DataValidation.Changelist.WorldPartition.InvalidRuntimeGrid", "Actor {0} has an invalid runtime grid {1}"),
											FText::FromString(GetFullActorName(ActorDescView)), 
											FText::FromName(GridName));

		Errors.Add(CurrentError);
	}
}

void FWorldPartitionSourceControlValidator::OnInvalidReference(const FWorldPartitionActorDescView& ActorDescView, const FGuid& ReferenceGuid, FWorldPartitionActorDescView* ReferenceActorDescView)
{
	if (RelevantToSourceControl(ActorDescView))
	{
		FText CurrentError = FText::Format(LOCTEXT("DataValidation.Changelist.WorldPartition.InvalidReference", "Actor {0} has an invalid reference to {1}"),
											FText::FromString(GetFullActorName(ActorDescView)), 
											FText::FromString(ReferenceActorDescView ? GetFullActorName(*ReferenceActorDescView) : ReferenceGuid.ToString()));

		Errors.Add(CurrentError);
	}
}

void FWorldPartitionSourceControlValidator::OnInvalidReferenceGridPlacement(const FWorldPartitionActorDescView& ActorDescView, const FWorldPartitionActorDescView& ReferenceActorDescView)
{
	if (RelevantToSourceControl(ActorDescView) || RelevantToSourceControl(ReferenceActorDescView))
	{
		// Only report errors for non-spatially loaded actor referencing a spatially loaded actor
		if (!ActorDescView.GetIsSpatiallyLoaded())
		{
			const FString SpatiallyLoadedActor(TEXT("Spatially loaded actor"));
			const FString NonSpatiallyLoadedActor(TEXT("Non-spatially loaded loaded actor"));

			FText CurrentError = FText::Format(LOCTEXT("DataValidation.Changelist.WorldPartition.InvalidReferenceGridPlacement", "{0} {1} is referencing {2} {3}."),
												FText::FromString(ActorDescView.GetIsSpatiallyLoaded() ? *SpatiallyLoadedActor : *NonSpatiallyLoadedActor),
												FText::FromString(GetFullActorName(ActorDescView)),
												FText::FromString(ReferenceActorDescView.GetIsSpatiallyLoaded() ? *SpatiallyLoadedActor : *NonSpatiallyLoadedActor),
												FText::FromString(GetFullActorName(ReferenceActorDescView)));

			Errors.Add(CurrentError);
		}
	}
}

void FWorldPartitionSourceControlValidator::OnInvalidReferenceDataLayers(const FWorldPartitionActorDescView& ActorDescView, const FWorldPartitionActorDescView& ReferenceActorDescView)
{
	if (RelevantToSourceControl(ActorDescView) || RelevantToSourceControl(ReferenceActorDescView))
	{
		FText CurrentError = FText::Format(LOCTEXT("DataValidation.Changelist.WorldPartition.DataLayerError", "{0} is referencing {1} but both actors are using a different set of runtime data layers."),
											FText::FromString(GetFullActorName(ActorDescView)),
											FText::FromString(GetFullActorName(ReferenceActorDescView)));

		Errors.Add(CurrentError);
	}
}

void FWorldPartitionSourceControlValidator::OnInvalidReferenceLevelScriptStreamed(const FWorldPartitionActorDescView& ActorDescView)
{
	if (RelevantToSourceControl(ActorDescView))
	{		
		FText CurrentError = FText::Format(LOCTEXT("DataValidation.Changelist.WorldPartition.InvalidReferenceLevelScriptStreamed", "Level script blueprint references streamed actor {0}."),
											FText::FromString(GetFullActorName(ActorDescView)));
		
		Errors.Add(CurrentError);
	}
}

void FWorldPartitionSourceControlValidator::OnInvalidReferenceLevelScriptDataLayers(const FWorldPartitionActorDescView& ActorDescView)
{
	if (RelevantToSourceControl(ActorDescView))
	{
		FText CurrentError = FText::Format(LOCTEXT("DataValidation.Changelist.WorldPartition.InvalidReferenceLevelScriptDataLayers", "Level script blueprint references streamed actor {0} with a non empty set of data layers."),
											FText::FromString(GetFullActorName(ActorDescView)));

		Errors.Add(CurrentError);
	}
}

void FWorldPartitionSourceControlValidator::OnInvalidReferenceRuntimeGrid(const FWorldPartitionActorDescView& ActorDescView, const FWorldPartitionActorDescView& ReferenceActorDescView)
{
	if (RelevantToSourceControl(ActorDescView) || RelevantToSourceControl(ReferenceActorDescView))
	{
		FText CurrentError = FText::Format(LOCTEXT("DataValidation.Changelist.WorldPartition.RuntimeGridError", "{0} is referencing {1} but both actors are using a different runtime grid."),
			FText::FromString(GetFullActorName(ActorDescView)),
			FText::FromString(GetFullActorName(ReferenceActorDescView)));

		Errors.Add(CurrentError);
	}
}

void FWorldPartitionSourceControlValidator::OnInvalidReferenceDataLayerAsset(const UDataLayerInstanceWithAsset* DataLayerInstance)
{
	if (Params.bHasWorldDataLayers)
	{
		FText CurrentError = FText::Format(LOCTEXT("DataValidation.Changelist.WorldPartition.InvalidDataLayerAsset", "Data layer {0} has no data layer asset."),
			FText::FromName(DataLayerInstance->GetDataLayerFName()));

		Errors.Add(CurrentError);
	}
}

void FWorldPartitionSourceControlValidator::OnDataLayerHierarchyTypeMismatch(const UDataLayerInstance* DataLayerInstance, const UDataLayerInstance* Parent)
{
	if (RelevantToSourceControl(DataLayerInstance) || RelevantToSourceControl(Parent) || Params.bHasWorldDataLayers)
	{
		FText CurrentError = FText::Format(LOCTEXT("DataValidation.Changelist.WorldPartition.DataLayerHierarchyTypeMismatch", "Data layer {0} is of type {1} and its parent {2} is of type {3}."),
			FText::FromString(DataLayerInstance->GetDataLayerFullName()),
			UEnum::GetDisplayValueAsText(DataLayerInstance->GetType()),
			FText::FromString(Parent->GetDataLayerFullName()),
			UEnum::GetDisplayValueAsText(Parent->GetType()));
	
		Errors.Add(CurrentError);
	}
}

void FWorldPartitionSourceControlValidator::OnDataLayerAssetConflict(const UDataLayerInstanceWithAsset* DataLayerInstance, const UDataLayerInstanceWithAsset* ConflictingDataLayerInstance)
{
	if (RelevantToSourceControl(DataLayerInstance) || RelevantToSourceControl(ConflictingDataLayerInstance) || Params.bHasWorldDataLayers)
	{
		FText CurrentError = FText::Format(LOCTEXT("DataValidation.Changelist.WorldPartition.DataLayerAssetConflict", "Data layer instance {0} and data layer instance {1} are both referencing data layer asset {2}."),
			FText::FromName(DataLayerInstance->GetDataLayerFName()),
			FText::FromName(ConflictingDataLayerInstance->GetDataLayerFName()),
			FText::FromString(DataLayerInstance->GetAsset()->GetFullName()));

		Errors.Add(CurrentError);
	}
}

void FWorldPartitionSourceControlValidator::OnActorNeedsResave(const FWorldPartitionActorDescView& ActorDescView)
{
	// Changelist validation already ensures that dirty actors must be part of the changelist
}

void FWorldPartitionSourceControlValidator::OnLevelInstanceInvalidWorldAsset(const FWorldPartitionActorDescView& ActorDescView, FName WorldAsset, ELevelInstanceInvalidReason Reason)
{
	if (RelevantToSourceControl(ActorDescView))
	{
		FText CurrentError;

		switch (Reason)
		{
		case ELevelInstanceInvalidReason::WorldAssetNotFound:
			CurrentError = FText::Format(LOCTEXT("DataValidation.Changelist.WorldPartition.LevelInstanceInvalidWorldAsset", "Level instance {0} has an invalid world asset {1}."),
				FText::FromString(GetFullActorName(ActorDescView)), 
				FText::FromName(WorldAsset));
			Errors.Add(CurrentError);
			break;
		case ELevelInstanceInvalidReason::WorldAssetNotUsingExternalActors:
			// Not a validation error
			break;
		case ELevelInstanceInvalidReason::WorldAssetImcompatiblePartitioned:
			// Not a validation error
			break;
		case ELevelInstanceInvalidReason::WorldAssetHasInvalidContainer:
			// We cannot treat that error as a validation error as it's possible to validate changelists without loading the world
			break;
		};
	}
}

#undef LOCTEXT_NAMESPACE