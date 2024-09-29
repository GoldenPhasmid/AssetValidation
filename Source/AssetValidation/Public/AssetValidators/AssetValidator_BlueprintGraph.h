#pragma once

#include "AssetValidator.h"

#include "AssetValidator_BlueprintGraph.generated.h"

class UEdGraphPin;
class UEdGraphNode;
class UBlueprint;

/**
 * Blueprint Graph Validator
 * Analyzes blueprint graph nodes and logs any banned functions, nodes or property types.										
 * Marked as EditInlineNew for commandlet actions
 */
UCLASS(EditInlineNew)
class ASSETVALIDATION_API UAssetValidator_BlueprintGraph: public UAssetValidator
{
	GENERATED_BODY()
public:

	UAssetValidator_BlueprintGraph();
	
	//~Begin EditorValidatorBase
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
	//~End EditorValidatorBase
	
	EDataValidationResult ValidateBlueprint(const UBlueprint* Blueprint, const FAssetData& AssetData, FDataValidationContext& Context, TArray<UEdGraphNode*>& OutInvalidNodes) const;
	EDataValidationResult ValidateNode(const UBlueprint* Blueprint, UEdGraphNode* Node, const FAssetData& AssetData, FDataValidationContext& Context) const;
	EDataValidationResult ValidateProperty(const UBlueprint* Blueprint, const FProperty* Property, const FAssetData& AssetData, FDataValidationContext& Context) const;
	
	static bool ValidatePin(const UEdGraphPin* Pin);
	
	static bool IsBlueprintGeneratedClass(const UClass* Class);
	static void HighlightNode(const UEdGraphNode* Node, const FText& Msg, bool bNotifyGraphUpdated = false);
	
	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation", meta = (Validate))
	TArray<FString> BannedFunctions;

	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation", meta = (Validate))
	TArray<TSubclassOf<UK2Node>> BannedFunctionPins;

	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation")
	bool bValidateBannedFunctions = true;

	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation")
	bool bValidateBlueprintCasts = true;

	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation")
	bool bValidateFunctionPins = true;

	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation")
	bool bValidateFunctionLocalVariables = true;

	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation")
	bool bValidateBlueprintVariables = true;
};
