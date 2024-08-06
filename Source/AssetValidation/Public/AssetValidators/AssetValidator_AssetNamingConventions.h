#pragma once

#include "CoreMinimal.h"
#include "AssetValidator.h"

#include "AssetValidator_AssetNamingConventions.generated.h"

UCLASS()
class UAssetValidator_AssetNamingConventions: public UAssetValidator
{
	GENERATED_BODY()
public:

	UAssetValidator_AssetNamingConventions();
	
	//~Begin EditorValidatorBase interface
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateAsset_Implementation(const FAssetData& InAssetData, FDataValidationContext& InContext) override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext) override;
	//~End EditorValidatorBase interface
};
