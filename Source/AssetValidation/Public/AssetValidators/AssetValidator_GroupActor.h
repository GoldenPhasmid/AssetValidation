#pragma once

#include "CoreMinimal.h"
#include "AssetValidator.h"

#include "AssetValidator_GroupActor.generated.h"

UCLASS()
class UAssetValidator_GroupActor: public UAssetValidator
{
	GENERATED_BODY()
public:

	UAssetValidator_GroupActor();

	//~Begin EditorValidatorBase interface
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext) override;
	//~End EditorValidatorBase interface

protected:

	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation", meta = (Validate))
	FText GroupActorSubmitFailedText;
	
	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation", meta = (Validate))
	FText ActorInGroupSubmitFailedText;

	/** A list of worlds that this validator should be applied to */
	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation", meta = (Validate, MetaClass = "/Script/Engine.World"))
	TArray<FSoftObjectPath> WorldFilter;
};
