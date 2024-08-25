#pragma once

#include "CoreMinimal.h"
#include "AVCommandletAction.h"

#include "AVCommandletAction_FindNodesAndExecuteAction.generated.h"

class UAssetValidator_BlueprintGraph;

UENUM()
enum class ENodeAction: uint8
{
	Count	UMETA(DisplayName = "Enumerate Nodes"),
	Refresh UMETA(DisplayName = "Refresh Nodes"),
	Delete	UMETA(DisplayName = "Delete Nodes"),
};

UCLASS(DisplayName = "Find Blueprint Nodes And Execute Action")
class UAVCommandletAction_FindNodesAndExecuteAction: public UAVCommandletAction
{
	GENERATED_BODY()
public:

	virtual void PostInitProperties() override;
	
	//~Begin CommandletAction interface
	virtual void InitFromCommandlet(const TArray<FString>& Switches, const TMap<FString, FString>& Params) override;
	virtual bool Run(const TArray<FAssetData>& Assets) override;
	//~End CommandletAction interface
	
	UPROPERTY(EditAnywhere, Instanced, Category = "Action")
	TObjectPtr<UAssetValidator_BlueprintGraph> Validator;

	UPROPERTY(EditAnywhere, Category = "Action")
	ENodeAction NodeAction = ENodeAction::Count;
};
