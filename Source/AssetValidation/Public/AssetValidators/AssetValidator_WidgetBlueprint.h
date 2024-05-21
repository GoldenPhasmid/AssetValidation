#pragma once

#include "CoreMinimal.h"
#include "AssetValidators/AssetValidator.h"

#include "AssetValidator_WidgetBlueprint.generated.h"

UCLASS()
class ASSETVALIDATION_API UAssetValidator_WidgetBlueprint: public UAssetValidator
{
	GENERATED_BODY()
public:
	UAssetValidator_WidgetBlueprint();
	
	//~Begin EditorValidatorBase interface
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
	//~End EditorValidatorBase interface
};
