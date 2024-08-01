#pragma once

#include "CoreMinimal.h"
#include "AssetValidator.h"

#include "AssetValidator_BSP.generated.h"

UCLASS()
class UAssetValidator_BSP: public UAssetValidator
{
	GENERATED_BODY()
public:
	UAssetValidator_BSP();
	
	//~Begin EditorValidatorBase interface
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext) override;
	//~End EditorValidatorBase interface

protected:

	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation", meta = (Validate))
	FText LevelHasBSPSubmitFailedText;

	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation", meta = (Validate))
	FText BSPBrushSubmitFailedText;

	/** A list of worlds that this validator should be applied to */
	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation", meta = (Validate, MetaClass = "/Script/Engine.World"))
	TArray<FSoftObjectPath> WorldFilter;

	
};
