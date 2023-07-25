// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetValidator.h"

#include "AssetValidator_AnimSequence.generated.h"

UCLASS()
class ASSETVALIDATION_API UAssetValidator_AnimSequence: public UAssetValidator
{
	GENERATED_BODY()
public:
	
	virtual bool CanValidateAsset_Implementation(UObject* InAsset) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors) override;

protected:

	bool ContainsCurve(const USkeleton* Skeleton, FName CurveName) const;
};
