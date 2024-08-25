#pragma once

#include "CoreMinimal.h"
#include "AssetValidators/AssetValidator.h"

#include "AssetValidator_AnimationAsset.generated.h"

UCLASS()
class ASSETVALIDATION_API UAssetValidator_AnimationAsset: public UAssetValidator
{
	GENERATED_BODY()
public:
	//~Begin EditorValidatorBase interface
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
	//~End EditorValidatorBase interface

	static EDataValidationResult ValidateSkeleton(const USkeleton* Skeleton, const UAnimationAsset& AnimAsset, const FAssetData& AssetData, FDataValidationContext& Context);
	static EDataValidationResult ValidateAnimNotifies(const UAnimationAsset& AnimAsset, const FAssetData& AssetData, FDataValidationContext& Context);
	static EDataValidationResult ValidateCurves(const UAnimationAsset& AnimAsset, const FAssetData& AssetData, FDataValidationContext& Context);
	static bool CurveExists(const USkeleton& Skeleton, FName CurveName);
};
