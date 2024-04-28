#pragma once

#include "CoreMinimal.h"
#include "DataValidationModule.h"
#include "AssetValidators/AssetValidator.h"

#include "AssetValidator_WorldActors.generated.h"

UCLASS()
class UAssetValidator_WorldActors: public UAssetValidator
{
	GENERATED_BODY()
public:

	virtual bool CanValidate_Implementation(const EDataValidationUsecase InUsecase) const override;
	virtual bool CanValidateAsset_Implementation(UObject* InAsset) const override;
	virtual EDataValidationResult ValidateAsset(const FAssetData& AssetData, TArray<FText>& ValidationErrors) override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors) override;

protected:

	EDataValidationResult ValidateWorld(UWorld* World, TArray<FText>& ValidationErrors);

	EDataValidationUsecase CurrentUseCase = EDataValidationUsecase::None;
	bool bRecursiveGuard = false;
};
