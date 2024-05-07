#pragma once

#include "AssetValidator.h"

#include "AssetValidator_SourceControl.generated.h"

UCLASS()
class UAssetValidator_SourceControl: public UAssetValidator
{
	GENERATED_BODY()
public:

	//~Begin EditorValidatorBase interface
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
	//~End EditorValidatorBase interface
	
	UPROPERTY(EditAnywhere, Config)
	bool bIgnoreEngineDependencies = true;
};
