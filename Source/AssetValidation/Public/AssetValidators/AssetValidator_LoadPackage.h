#pragma once

#include "AssetValidator.h"

#include "AssetValidator_LoadPackage.generated.h"

UCLASS()
class ASSETVALIDATION_API UAssetValidator_LoadPackage: public UAssetValidator
{
	GENERATED_BODY()
public:

	//~Begin EditorValidatorBase interface
	virtual bool IsEnabled() const override;
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
	//~End EditorValidatorBase interface

	/** Load package */
	static bool GetPackageLoadErrors(const FString& PackageName, TArray<FString>& OutWarnings, TArray<FString>& OutErrors);
};
