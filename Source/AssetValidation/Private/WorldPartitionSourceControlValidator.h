#pragma once

#include "AssetValidationDefines.h"

#if !WITH_DATA_VALIDATION_UPDATE

#include "WorldPartition/ErrorHandling/WorldPartitionStreamingGenerationTokenizedMessageErrorHandler.h"

struct FWorldPartitionValidatorParams
{
	FTopLevelAssetPath RelevantMap;
	TSet<FGuid> RelevantActorGuids;
	TSet<FString> RelevantDataLayerAssets;
	bool bHasWorldDataLayers = false;
};

/**
 * Gathers errors generated during UWorldPartition::CheckForErrors
 * This is a functionality copy from UWorldPartitionChangelistValidator
 */
class FWorldPartitionSourceControlValidator: public IStreamingGenerationErrorHandler
{
	FWorldPartitionValidatorParams Params;
	TArray<FText> Errors;
public:

	void Initialize(FWorldPartitionValidatorParams&& InParams);

	TArray<FText> GetErrors() const;

	/** @return true if this ActorDescView is pertinent to the source control */
	bool RelevantToSourceControl(const FWorldPartitionActorDescView& ActorDescView);
	/** @return true if this UDataLayerInstance is pertinent to the source control */
	bool RelevantToSourceControl(const UDataLayerInstance* DataLayerInstance);
	
	//~Begin IStreamingGenerationErrorHandler
	virtual void OnInvalidRuntimeGrid(const FWorldPartitionActorDescView& ActorDescView, FName GridName) override;
	virtual void OnInvalidReference(const FWorldPartitionActorDescView& ActorDescView, const FGuid& ReferenceGuid, FWorldPartitionActorDescView* ReferenceActorDescView) override;
	virtual void OnInvalidReferenceGridPlacement(const FWorldPartitionActorDescView& ActorDescView, const FWorldPartitionActorDescView& ReferenceActorDescView) override;
	virtual void OnInvalidReferenceDataLayers(const FWorldPartitionActorDescView& ActorDescView, const FWorldPartitionActorDescView& ReferenceActorDescView) override;
	virtual void OnInvalidReferenceLevelScriptStreamed(const FWorldPartitionActorDescView& ActorDescView) override;
	virtual void OnInvalidReferenceLevelScriptDataLayers(const FWorldPartitionActorDescView& ActorDescView) override;
	virtual void OnInvalidReferenceRuntimeGrid(const FWorldPartitionActorDescView& ActorDescView, const FWorldPartitionActorDescView& ReferenceActorDescView) override;
	virtual void OnInvalidReferenceDataLayerAsset(const UDataLayerInstanceWithAsset* DataLayerInstance) override;
	virtual void OnDataLayerHierarchyTypeMismatch(const UDataLayerInstance* DataLayerInstance, const UDataLayerInstance* Parent) override;
	virtual void OnDataLayerAssetConflict(const UDataLayerInstanceWithAsset* DataLayerInstance, const UDataLayerInstanceWithAsset* ConflictingDataLayerInstance) override;
	virtual void OnActorNeedsResave(const FWorldPartitionActorDescView& ActorDescView) override;
	virtual void OnLevelInstanceInvalidWorldAsset(const FWorldPartitionActorDescView& ActorDescView, FName WorldAsset, ELevelInstanceInvalidReason Reason) override;
	virtual void OnInvalidActorFilterReference(const FWorldPartitionActorDescView& ActorDescView, const FWorldPartitionActorDescView& ReferenceActorDescView) override;
	//~End IStreamingGenerationErrorHandler
};
#endif

