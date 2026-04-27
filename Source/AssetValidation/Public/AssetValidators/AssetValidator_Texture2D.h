#pragma once

#include "CoreMinimal.h"
#include "AssetValidator.h"

#include "AssetValidator_Texture2D.generated.h"

UCLASS(Config = Editor, DefaultConfig)
class ASSETVALIDATION_API UAssetValidator_Texture2D: public UAssetValidator
{
	GENERATED_BODY()
public:
	UAssetValidator_Texture2D();

	virtual bool IsEnabled() const override;
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;

	/** set to 0 to allow any UI texture to not have compression */
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Asset Validation")
	int32 MaxTextureSizeWithoutCompression = 512;
};
