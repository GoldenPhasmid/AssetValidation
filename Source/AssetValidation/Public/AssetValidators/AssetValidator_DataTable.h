#pragma once

#include "CoreMinimal.h"
#include "AssetValidator.h"

#include "AssetValidator_DataTable.generated.h"

UCLASS()
class ASSETVALIDATION_API UAssetValidator_DataTable: public UAssetValidator
{
	GENERATED_BODY()
public:

	UAssetValidator_DataTable();
	
	//~Begin EditorValidatorBase interface
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
	//~End EditorValidatorBase interface
};
