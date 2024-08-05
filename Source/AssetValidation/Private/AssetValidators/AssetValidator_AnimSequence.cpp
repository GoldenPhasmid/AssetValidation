
#include "AssetValidators/AssetValidator_AnimSequence.h"

#include "AssetValidationDefines.h"
#include "AssetValidationModule.h"
#include "AssetValidationStatics.h"
#include "AssetRegistry/AssetDataToken.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

bool UAssetValidator_AnimSequence::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	return Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext) && InObject && InObject->IsA<UAnimSequenceBase>();
}

EDataValidationResult UAssetValidator_AnimSequence::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_AnimSequence, AssetValidationChannel);
	
	const UAnimSequenceBase* AnimSequence = CastChecked<UAnimSequenceBase>(InAsset);
	const USkeleton* Skeleton = AnimSequence->GetSkeleton();
	
	if (!IsValid(Skeleton))
	{
		Context.AddMessage(UE::AssetValidation::CreateTokenMessage(
			EMessageSeverity::Error, InAssetData, LOCTEXT("NoSkeleton", "animation is missing skeleton.")
		));
		return EDataValidationResult::Invalid;
	}

	EDataValidationResult Result = ValidateCurves(*Skeleton, *AnimSequence, InAssetData, Context);
	if (Result != EDataValidationResult::Invalid)
	{
		AssetPasses(AnimSequence);
	}

	return Result;
}

EDataValidationResult UAssetValidator_AnimSequence::ValidateCurves(const USkeleton& Skeleton, const UAnimSequenceBase& AnimSequence, const FAssetData& AssetData, FDataValidationContext& Context)
{
	const FAssetData SkeletonAsset{&Skeleton};
	const TArray<FFloatCurve>& Curves = AnimSequence.GetCurveData().FloatCurves;

	EDataValidationResult Result = EDataValidationResult::Valid;
	for (const FFloatCurve& Curve: Curves)
	{
		const FName CurveName = Curve.GetName();
		if (!CurveExists(Skeleton, CurveName))
		{
			Context.AddMessage(UE::AssetValidation::CreateTokenMessage(EMessageSeverity::Error, AssetData,
				FText::Format(LOCTEXT("NoCurveFormat", "No curve [{0}] present on skeleton "), FText::FromName(CurveName)),
				SkeletonAsset, LOCTEXT("CurveExistsOnAsset", ", but exists on animation sequence.")
			));
			Result &= EDataValidationResult::Invalid;
		}
	}

	return Result;
}

bool UAssetValidator_AnimSequence::CurveExists(const USkeleton& Skeleton, FName CurveName)
{
	return Skeleton.GetCurveMetaData(CurveName) != nullptr;
}

#undef LOCTEXT_NAMESPACE
