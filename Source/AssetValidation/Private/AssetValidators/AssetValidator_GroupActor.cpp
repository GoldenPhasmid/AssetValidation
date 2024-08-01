#include "AssetValidators/AssetValidator_GroupActor.h"

#include "Editor/GroupActor.h"
#include "Misc/DataValidation.h"
#include "AssetValidationDefines.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

UAssetValidator_GroupActor::UAssetValidator_GroupActor()
{
	GroupActorSubmitFailedText = LOCTEXT(
		"AssetValidator_GroupActor",
		"Submitting group actors is not allowed. Please ungroup all actors before submitting to the source control."
	);
	ActorInGroupSubmitFailedText = LOCTEXT(
		"AssetValidator_ActorInGroup",
		"Actor is part of an active group. Please ungroup all actors before submitting to the source control."
	);
	bIsConfigDisabled = true; // disabled by default
}

bool UAssetValidator_GroupActor::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	if (!Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext))
	{
		return false;
	}
	
	if (InContext.GetValidationUsecase() != EDataValidationUsecase::PreSubmit)
	{
		return false;
	}

	const AActor* Actor = Cast<AActor>(InObject);
	if (Actor == nullptr)
	{
		return false;
	}
	
	FSoftObjectPath OuterAssetPath{InAssetData.GetSoftObjectPath().GetAssetPath(), {}};
	if (OuterAssetPath.IsNull() || (WorldFilter.Num() > 0 && !WorldFilter.Contains(OuterAssetPath)))
	{
		return false;
	}

	return true;
}

EDataValidationResult UAssetValidator_GroupActor::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext)
{
	check(InAsset);
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_GroupActor, AssetValidationChannel);
	
	if (InAsset->IsA<AGroupActor>())
	{
		InContext.AddMessage(InAssetData, EMessageSeverity::Error, GroupActorSubmitFailedText);
		return EDataValidationResult::Invalid;
	}
	else if (AActor* Actor = Cast<AActor>(InAsset); Actor && Actor->GroupActor != nullptr)
	{
		InContext.AddMessage(InAssetData, EMessageSeverity::Error, ActorInGroupSubmitFailedText);
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}

#undef LOCTEXT_NAMESPACE
