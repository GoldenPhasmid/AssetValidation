#pragma once

#include "AssetValidator.h"

#include "AssetValidator_SourceControl.generated.h"

UCLASS()
class UAssetValidator_SourceControl: public UAssetValidator
{
	GENERATED_BODY()
public:

	virtual bool CanValidateAsset_Implementation(UObject* InAsset) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors) override;
};
