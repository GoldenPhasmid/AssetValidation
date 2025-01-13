#pragma once

#include "AssetValidator.h"

#include "AssetValidator_BlueprintGraph.generated.h"

class UK2Node_Event;
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
	static bool IsLeafEventNode(const UK2Node_Event* Node);
	
	static bool IsBlueprintGeneratedClass(const UClass* Class);
	static void HighlightNode(const UEdGraphNode* Node, const FText& Msg, bool bNotifyGraphUpdated = false);

	/** List of banned functions used by @bValidateBannedFunctions option */
	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation", meta = (Validate))
	TArray<FString> BannedFunctions;

	/** List of node types used by @bValidateFunctionPins option */
	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation", meta = (Validate))
	TArray<TSubclassOf<UK2Node>> BannedFunctionPins;

	/**
	 * If enabled, will report using @BannedFunctions as compilation errors.
	 * This is a way to disable usage of built-in engine functionality without changing the engine
	 * Good example can be disabling GetAllActors function family and providing an alternatives for different use cases
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation")
	bool bValidateBannedFunctions = true;

	/** If enabled, casts to non-abstract blueprint generated classes will be reported as compilation errors */
	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation")
	bool bValidateBlueprintCasts = true;

	/** If enabled, will report pins with blueprint generated class type as compilation errors. Only checks node types from @BannedFunctionPins list */
	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation")
	bool bValidateFunctionPins = true;

	/**
	 * If enabled, runs property validation on the blueprint variables.
	 * Properties with a type of blueprint generated class are reported as compilation errors
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation")
	bool bValidateBlueprintVariables = true;

	/**
	 * If enabled, runs property validation on the blueprint function local variables.
	 * Properties with a type of blueprint generated class are reported as compilation errors
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation")
	bool bValidateFunctionLocalVariables = true;
	
	/** If enabled, empty tick functions are reported as errors. */
	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation")
	bool bValidateEmptyTickFunctions = true;

	/**
	 * If enabled, will report blueprint pure nodes with multiple connected output pins as errors
	 * MultiPin pure nodes get called for each connected pin output, which causes overhead when pure functions used for complex calculations
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation")
	bool bValidateMultiPinPureNodes = true;
};
