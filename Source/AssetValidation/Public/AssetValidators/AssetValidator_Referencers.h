#pragma once

#include "CoreMinimal.h"
#include "AssetValidators/AssetValidator.h"

#include "AssetValidator_Referencers.generated.h"

UCLASS()
class ASSETVALIDATION_API UAssetValidator_Referencers: public UAssetValidator
{
	GENERATED_BODY()
public:
	//~Begin	EditorValidatorBase interface
	virtual bool IsEnabled() const override;
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
	//~End		EditorValidatorBase interface
};
