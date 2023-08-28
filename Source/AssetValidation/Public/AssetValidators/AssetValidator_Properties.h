#pragma once

#include "AssetValidator.h"

#include "AssetValidator_Properties.generated.h"

UCLASS(Config = Editor)
class ASSETVALIDATION_API UAssetValidator_Properties: public UAssetValidator
{
	GENERATED_BODY()
public:

	//~Begin EditorValidatorBase 
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors) override;
	//~End EditorValidatorBase

};
