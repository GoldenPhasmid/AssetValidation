#pragma once

#include "CoreMinimal.h"
#include "AssetValidator.h"

#include "AssetValidator_DataTable.generated.h"

UCLASS()
class ASSETVALIDATION_API UAssetValidator_DataTable: public UAssetValidator
{
	GENERATED_BODY()
public:

	//~Begin EditorValidatorBase
	virtual bool CanValidateAsset_Implementation(UObject* InAsset) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors) override;
	//~End EditorValidatorBase

	
};
