#pragma once

#include "CoreMinimal.h"
#include "Commandlet/AVCommandletAction.h"
#include "AssetValidators/AssetValidator.h"

#include "AVCommandletAction_ValidateAssets.generated.h"

class UAssetValidator;
enum class EDataValidationUsecase: uint8;

UCLASS(DisplayName = "Validate Assets")
class ASSETVALIDATION_API UAVCommandletAction_ValidateAssets: public UAVCommandletAction
{
	GENERATED_BODY()
public:

	UAVCommandletAction_ValidateAssets();
	
	//~Begin AVCommandletAction interface
	virtual void InitFromCommandlet(const TArray<FString>& Switches, const TMap<FString, FString>& Params) override;
	virtual bool Run(const TArray<FAssetData>& Assets) override;
	//~End AVCommandletAction interface

	void DisableValidators(TArray<UEditorValidatorBase*>& OutDisabledValidators);
	void EnableValidators(TArrayView<UEditorValidatorBase*> Validators);

	UPROPERTY(EditAnywhere, Category = "Action")
	TArray<TSoftClassPtr<UAssetValidator>> DisabledValidators;

	UPROPERTY(EditAnywhere, Category = "Action")
	EDataValidationUsecase ValidationUsecase;
	
	UPROPERTY(EditAnywhere, Category = "Action")
	bool bDetailedLog = false;

	UPROPERTY(EditAnywhere, Category = "Action")
	bool bSkipExcludedDirectories = true;

	UPROPERTY()
	TSet<FName> CommandletDisabledValidators;
};
