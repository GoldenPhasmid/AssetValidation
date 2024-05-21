#pragma once

#include "CoreMinimal.h"
#include "AssetValidator.h"

#include "AssetValidator_AnimSequence.generated.h"

UCLASS()
class ASSETVALIDATION_API UAssetValidator_AnimSequence: public UAssetValidator
{
	GENERATED_BODY()
public:

	UAssetValidator_AnimSequence();
	
	//~Begin EditorValidatorBase interface
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
	//~End EditorValidatorBase interface

protected:

	bool CurveExists(const USkeleton* Skeleton, FName CurveName) const;
};
