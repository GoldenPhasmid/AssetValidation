#pragma once

#include "CoreMinimal.h"
#include "AssetValidator.h"

#include "AssetValidator_AnimSequence.generated.h"

UCLASS()
class ASSETVALIDATION_API UAssetValidator_AnimSequence: public UAssetValidator
{
	GENERATED_BODY()
public:
	
	//~Begin EditorValidatorBase interface
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
	//~End EditorValidatorBase interface

	static EDataValidationResult ValidateCurves(const USkeleton& Skeleton, const UAnimSequenceBase& AnimSequence, const FAssetData& AssetData, FDataValidationContext& Context);
	static bool CurveExists(const USkeleton& Skeleton, FName CurveName);
};
