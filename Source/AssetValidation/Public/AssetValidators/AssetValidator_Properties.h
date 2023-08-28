#pragma once

#include "AssetValidator.h"

#include "AssetValidator_Properties.generated.h"

UCLASS(Config = Editor)
class ASSETVALIDATION_API UAssetValidator_Properties: public UAssetValidator
{
	GENERATED_BODY()
public:

	UAssetValidator_Properties();

	//~Begin EditorValidatorBase 
	virtual bool CanValidateAsset_Implementation(UObject* InAsset) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors) override;
	//~End EditorValidatorBase

};
