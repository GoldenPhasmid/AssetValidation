#pragma once

#include "CoreMinimal.h"
#include "DataValidationModule.h"
#include "AssetValidators/AssetValidator.h"

#include "AssetValidator_World.generated.h"

class UAssetValidationSubsystem;

UCLASS()
class UAssetValidator_World: public UAssetValidator
{
	GENERATED_BODY()
public:

	//~Begin EditorValidatorBase interface
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateAsset_Implementation(const FAssetData& AssetData, FDataValidationContext& Context) override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext) override;
	//~End EditorValidatorBase interface
	
protected:

	EDataValidationResult ValidateWorld(const FAssetData& AssetData, UWorld* World, FDataValidationContext& Context);
	EDataValidationResult ValidateAssetInternal(UAssetValidationSubsystem& ValidationSubsystem, UObject* Asset, FDataValidationContext& Context);
	
	bool bRecursiveGuard = false;

	UPROPERTY(Config)
	bool bEnsureInactiveWorld = false;
};
