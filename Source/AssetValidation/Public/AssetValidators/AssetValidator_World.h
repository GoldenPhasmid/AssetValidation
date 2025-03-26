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
	UAssetValidator_World();

	//~Begin EditorValidatorBase interface
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateAsset_Implementation(const FAssetData& AssetData, FDataValidationContext& Context) override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext) override;
	//~End EditorValidatorBase interface
	
protected:

	EDataValidationResult ValidateWorld(const FAssetData& AssetData, UWorld* World, FDataValidationContext& Context);

	EDataValidationResult ValidateActor(const UAssetValidationSubsystem& ValidationSubsystem, AActor* Actor, FDataValidationContext& Context);
	EDataValidationResult ValidateObject(const UAssetValidationSubsystem& ValidationSubsystem, UObject* Object, FDataValidationContext& Context);

	EDataValidationResult ValidateExternalAssets(const FAssetData& InAssetData, FDataValidationContext& Context);
	/** @return approximate asset count that would be validated as part of world validation */
	int32 EstimateWorldAssetCount(const UWorld* World) const;
	
	bool bRecursiveGuard = false;
	UPROPERTY()
	UObject* CurrentExternalAsset = nullptr;

	UPROPERTY(EditAnywhere, Category = "Asset Validation")
	bool bEnsureInactiveWorld = false;

	UPROPERTY(EditAnywhere, Category = "Asset Validation")
	int32 ValidateOnSaveAssetCountThreshold = 800;
};
