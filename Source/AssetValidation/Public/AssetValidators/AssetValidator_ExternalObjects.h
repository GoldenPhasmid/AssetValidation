#pragma once

#include "CoreMinimal.h"
#include "AssetValidators/AssetValidator.h"

#include "AssetValidator_ExternalObjects.generated.h"

/**
 * External Object Validator
 * Routes asset validation for DataValidationContext external objects
 */
UCLASS(HideDropdown)
class UAssetValidator_ExternalObjects: public UAssetValidator
{
	GENERATED_BODY()
public:
	UAssetValidator_ExternalObjects();
	
	//~Begin EditorValidatorBase interface
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateAsset_Implementation(const FAssetData& InAssetData, FDataValidationContext& InContext) override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext) override;
	//~End EditorValidatorBase interface

protected:
	
	EDataValidationResult ValidateAssetInternal(const FAssetData& InAssetData, FDataValidationContext& InContext);
	
	UPROPERTY()
	UObject* CurrentExternalAsset = nullptr;
};
