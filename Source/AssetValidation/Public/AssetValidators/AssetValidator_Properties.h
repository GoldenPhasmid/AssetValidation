#pragma once

#include "AssetValidator.h"
#include "AssetValidationDefines.h"

#include "AssetValidator_Properties.generated.h"

UCLASS(Config = Editor)
class ASSETVALIDATION_API UAssetValidator_Properties: public UAssetValidator
{
	GENERATED_BODY()
public:

	//~Begin EditorValidatorBase
#if WITH_DATA_VALIDATION_UPDATE
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
#else
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors) override;
#endif
	//~End EditorValidatorBase
};
