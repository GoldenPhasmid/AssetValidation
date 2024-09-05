#pragma once

#include "CoreMinimal.h"
#include "AssetValidator.h"

#include "AssetValidator_TickFunction.generated.h"

UCLASS()
class UAssetValidator_TickFunction: public UAssetValidator
{
	GENERATED_BODY()
public:
	
	UAssetValidator_TickFunction();
	
	//~Begin EditorValidatorBase
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
	//~End EditorValidatorBase
};
