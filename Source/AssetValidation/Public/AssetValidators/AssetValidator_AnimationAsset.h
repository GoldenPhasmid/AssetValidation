#pragma once

#include "CoreMinimal.h"
#include "AssetValidators/AssetValidator.h"

#include "AssetValidator_AnimationAsset.generated.h"

UCLASS()
class ASSETVALIDATION_API UAssetValidator_AnimationAsset: public UAssetValidator
{
	GENERATED_BODY()
public:
	UAssetValidator_AnimationAsset();
	
	//~Begin EditorValidatorBase interface
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
	//~End EditorValidatorBase interface

	EDataValidationResult ValidateRootMotion(const UAnimSequence& AnimSequence, FDataValidationContext& Context) const;
	static EDataValidationResult ValidateSkeleton(const USkeleton* Skeleton, const UAnimationAsset& AnimAsset, const FAssetData& AssetData, FDataValidationContext& Context);
	static EDataValidationResult ValidateAnimNotifies(const UAnimationAsset& AnimAsset, const FAssetData& AssetData, FDataValidationContext& Context);
	static EDataValidationResult ValidateCurves(const UAnimationAsset& AnimAsset, const FAssetData& AssetData, FDataValidationContext& Context);
	static bool CurveExists(const USkeleton& Skeleton, FName CurveName);

	UFUNCTION(BlueprintPure, Category = "Asset Validation")
	static bool SkeletonHasCurve(const USkeleton* Skeleton, FName CurveName);
	
	UFUNCTION(BlueprintCallable, Category = "Asset Validation")
	static bool AddCurveToSkeleton(USkeleton* Skeleton, FName CurveName);
	
	UPROPERTY(EditAnywhere, Config, Category = "Asset Validation")
	bool bAllowNamedAnimNotifies = true;

	UPROPERTY(EditAnywhere, Config, Category = "Asset Validation")
	bool bValidateRootMotion = true;

	/** Verifies that animation Up vector always matches the Up Axis */
	UPROPERTY(EditAnywhere, Config, Category = "Asset Validation", meta = (EditCondition = "bValidateUpAxis"))
	FVector RootMotionUpAxis = FVector::UpVector;
};
