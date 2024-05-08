
#include "AssetValidators/AssetValidator_AnimSequence.h"

#include "AssetValidationDefines.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

bool UAssetValidator_AnimSequence::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	return Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext) && InObject && InObject->IsA<UAnimSequenceBase>();
}

EDataValidationResult UAssetValidator_AnimSequence::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_AnimSequence, AssetValidationChannel);
	
	UAnimSequenceBase* AnimAsset = CastChecked<UAnimSequenceBase>(InAsset);

	USkeleton* Skeleton = AnimAsset->GetSkeleton();
	check(Skeleton);
	
	const TArray<FFloatCurve>& Curves = AnimAsset->GetCurveData().FloatCurves;
	for (const FFloatCurve& Curve: Curves)
	{
		const FName CurveName = Curve.GetName();
		if (!CurveExists(Skeleton, CurveName))
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("CurveName"), FText::FromName(CurveName));
			Arguments.Add(TEXT("SkeletonName"), FText::FromString(Skeleton->GetName()));
			Arguments.Add(TEXT("AnimAsset"), FText::FromString(AnimAsset->GetName()));
			
			FText FailReason = FText::Format(FTextFormat::FromString(TEXT("No curve {CurveName} present on skeleton {SkeletonName}, but exists on anim asset {AnimAsset}")), Arguments);
			AssetFails(AnimAsset, FailReason);
		}
	}

	if (GetValidationResult() == EDataValidationResult::NotValidated)
	{
		AssetPasses(AnimAsset);
	}

	return GetValidationResult();
}

bool UAssetValidator_AnimSequence::CurveExists(const USkeleton* Skeleton, FName CurveName) const
{
	check(IsValid(Skeleton));

	// @todo: does it mean that curve exists? Need to test this
	return Skeleton->GetCurveMetaData(CurveName) != nullptr;
}

#undef LOCTEXT_NAMESPACE
