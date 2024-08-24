#pragma once

#include "CoreMinimal.h"
#include "Commandlet/AVCommandletAction.h"

#include "AVCommandletAction_ValidateAssets.generated.h"

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

	void DisableValidators();

	UPROPERTY(EditAnywhere, Category = "Action")
	TArray<TSoftClassPtr<UClass>> DisabledValidators;

	UPROPERTY(EditAnywhere, Category = "Action")
	EDataValidationUsecase ValidationUsecase;
	
	UPROPERTY(EditAnywhere, Category = "Action")
	bool bDetailedLog = false;

	UPROPERTY(EditAnywhere, Category = "Action")
	bool bSkipExcludedDirectories = true;
	
};
