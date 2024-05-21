#pragma once

#include "AssetValidator.h"
#include "AssetValidationDefines.h"

#include "AssetValidator_Properties.generated.h"

UCLASS(Config = Editor)
class ASSETVALIDATION_API UAssetValidator_Properties: public UAssetValidator
{
	GENERATED_BODY()
public:
	UAssetValidator_Properties();

	//~Begin EditorValidatorBase
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
	//~End EditorValidatorBase
};
