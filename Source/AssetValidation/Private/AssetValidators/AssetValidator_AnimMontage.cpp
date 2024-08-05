#include "AssetValidators/AssetValidator_AnimMontage.h"

#include "AssetValidationDefines.h"
#include "AssetValidationStatics.h"
#include "AssetValidators/AssetValidator_AnimSequence.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

bool UAssetValidator_AnimMontage::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	return Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext) && InObject && InObject->IsA<UAnimMontage>();
}

EDataValidationResult UAssetValidator_AnimMontage::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_AnimMontage, AssetValidationChannel);

	UAnimMontage* Montage = CastChecked<UAnimMontage>(InAsset);

	const USkeleton* Skeleton = Montage->GetSkeleton();
	if (!IsValid(Skeleton))
	{
		Context.AddMessage(UE::AssetValidation::CreateTokenMessage(
			EMessageSeverity::Error, InAssetData, LOCTEXT("NoSkeleton", "animation is missing skeleton.")
		));
		return EDataValidationResult::Invalid;
	}

	TArray<UAnimationAsset*> AnimationAssets;
	Montage->GetAllAnimationSequencesReferred(AnimationAssets);

	const FAssetData SkeletonAsset{Skeleton};
	EDataValidationResult Result = EDataValidationResult::Valid;
	for (const UAnimationAsset* AnimAsset: AnimationAssets)
	{
		if (AnimAsset->GetSkeleton() != Skeleton)
		{
			Context.AddMessage(UE::AssetValidation::CreateTokenMessage(
				EMessageSeverity::Error, InAssetData, LOCTEXT("NoSkeleton", "skeleton mismatch for animation asset "), FAssetData{AnimAsset}
			));
			Result &= EDataValidationResult::Invalid;
		}

		if (const UAnimSequenceBase* AnimSequence = Cast<UAnimSequenceBase>(AnimAsset))
		{
			Result &= UAssetValidator_AnimSequence::ValidateCurves(*Skeleton, *AnimSequence, InAssetData, Context);
		}
	}

	if (Result != EDataValidationResult::Invalid)
	{
		AssetPasses(Montage);
	}

	return Result;
}

#undef LOCTEXT_NAMESPACE
