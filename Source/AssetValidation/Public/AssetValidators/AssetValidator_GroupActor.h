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

	UPROPERTY(EditAnywhere)
	FText GroupActorSubmitFailedText;
	
	UPROPERTY(EditAnywhere)
	FText ActorInGroupSubmitFailedText;
};
