#pragma once

#include "AssetValidator.h"

#include "AssetValidator_LoadPackage.generated.h"

UCLASS(Config = Editor, DefaultConfig)
class ASSETVALIDATION_API UAssetValidator_LoadPackage: public UAssetValidator
{
	GENERATED_BODY()
public:

	UAssetValidator_LoadPackage();

	//~Begin EditorValidatorBase interface
	virtual bool IsEnabled() const override;
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
	//~End EditorValidatorBase interface

	/**
	 * Load package defined by @PackageName and catch any load associated errors and warnings
	 * @param PackageName package to validate
	 * @param AssetData asset data associated with check, can be invalid
	 * @param ValidationContext validation context
	 * @return true if package was loaded, false otherwise
	 */
	static bool GetPackageLoadErrors(const FString& PackageName, const FAssetData& AssetData, FDataValidationContext& ValidationContext);

	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation")
	TArray<FSoftClassPath> ClassPathsToIgnore;
};
