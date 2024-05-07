#pragma once

#include "CoreMinimal.h"
#include "AssetValidators/AssetValidator.h"

#include "AssetValidator_ExternalObjects.generated.h"

/**
 * External Object Validator
 * Routes asset validation for DataValidationContext external objects
 */
UCLASS()
class UAssetValidator_ExternalObjects: public UAssetValidator
{
	GENERATED_BODY()
public:
	//~Begin EditorValidatorBase interface
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext) override;
	//~End EditorValidatorBase interface

	UPROPERTY()
	UObject* CurrentExternalAsset = nullptr;
};
