// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetValidators/AssetValidator_AnimSequence.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

bool UAssetValidator_AnimSequence::CanValidateAsset_Implementation(UObject* InAsset) const
{
	return Super::CanValidateAsset_Implementation(InAsset) && InAsset && InAsset->IsA(UAnimSequenceBase::StaticClass());
}

EDataValidationResult UAssetValidator_AnimSequence::ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors)
{
	UAnimSequenceBase* AnimAsset = CastChecked<UAnimSequenceBase>(InAsset);

	USkeleton* Skeleton = AnimAsset->GetSkeleton();
	check(Skeleton);

	const TArray<FFloatCurve>& Curves = AnimAsset->GetCurveData().FloatCurves;
	for (const FFloatCurve& Curve: Curves)
	{
		const FName CurveName = Curve.Name.DisplayName;
		if (!ContainsCurve(Skeleton, CurveName))
		{
			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("CurveName"), FText::FromName(CurveName));
			Arguments.Add(TEXT("SkeletonName"), FText::FromString(Skeleton->GetName()));
			
			FText ErrorText = FText::Format(FTextFormat::FromString(TEXT("No curve {CurveName} present on skeleton {SkeletonName}")), Arguments);
			AssetFails(AnimAsset, ErrorText, ValidationErrors);
		}
	}

	if (GetValidationResult() == EDataValidationResult::NotValidated)
	{
		AssetPasses(AnimAsset);
	}

	return GetValidationResult();
}

bool UAssetValidator_AnimSequence::ContainsCurve(const USkeleton* Skeleton, FName CurveName) const
{
	check(IsValid(Skeleton));

	FSmartName SmartName;
	auto FindSmartName = [&](const FName& ContainerName)
	{
		const FSmartNameMapping* SmartNameContainer = Skeleton->GetSmartNameContainer(ContainerName);
		return SmartNameContainer->FindSmartName(CurveName, SmartName);
	};

	if (FindSmartName(USkeleton::AnimCurveMappingName) || FindSmartName(USkeleton::AnimTrackCurveMappingName))
	{
		return true;
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
