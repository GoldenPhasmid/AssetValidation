#pragma once

#include "AssetValidator.h"

#include "AssetValidator_LoadPackage.generated.h"

UCLASS()
class ASSETVALIDATION_API UAssetValidator_LoadPackage: public UAssetValidator
{
	GENERATED_BODY()
public:

	static bool GetPackageLoadErrors(const FString& PackageName, TArray<FString>& OutWarnings, TArray<FString>& OutErrors);
	
	virtual bool IsEnabled() const override;
	virtual bool CanValidate_Implementation(const EDataValidationUsecase InUsecase) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors) override;
};
