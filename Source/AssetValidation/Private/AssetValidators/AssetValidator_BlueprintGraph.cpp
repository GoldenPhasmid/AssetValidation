#include "AssetValidators/AssetValidator_BlueprintGraph.h"

#include "AssetValidationDefines.h"
#include "AssetValidationStatics.h"
#include "K2Node_CallFunction.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_Literal.h"
#include "K2Node_Select.h"
#include "EdGraph/EdGraphPin.h"
#include "Editor/BlueprintGraphToken.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

UAssetValidator_BlueprintGraph::UAssetValidator_BlueprintGraph()
{
	bIsConfigDisabled = true; // disabled by default

	bCanRunParallelMode = true;
	bRequiresLoadedAsset = true;
	bRequiresTopLevelAsset = true;
	bCanValidateActors = false;

	BannedFunctionPins.Add(UK2Node_CustomEvent::StaticClass());
	BannedFunctionPins.Add(UK2Node_FunctionEntry::StaticClass());
	BannedFunctionPins.Add(UK2Node_FunctionResult::StaticClass());
	BannedFunctionPins.Add(UK2Node_Literal::StaticClass());
	BannedFunctionPins.Add(UK2Node_Select::StaticClass());
}

bool UAssetValidator_BlueprintGraph::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	return Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext) && InObject != nullptr && InObject->IsA<UBlueprint>();
}

EDataValidationResult UAssetValidator_BlueprintGraph::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_BlueprintGraph, AssetValidationChannel);

	const UBlueprint* Blueprint = CastChecked<UBlueprint>(InAsset);
	if (FBlueprintEditorUtils::IsDataOnlyBlueprint(Blueprint))
	{
		AssetPasses(InAsset);
		return EDataValidationResult::Valid;
	}

	TArray<UEdGraphNode*> InvalidNodes;
	const EDataValidationResult Result = ValidateBlueprint(Blueprint, InAssetData, Context, InvalidNodes);
	
	if (Result != EDataValidationResult::Invalid)
	{
		AssetPasses(InAsset);
	}

	return Result;
}

EDataValidationResult UAssetValidator_BlueprintGraph::ValidateBlueprint(const UBlueprint* Blueprint, const FAssetData& AssetData, FDataValidationContext& Context, TArray<UEdGraphNode*>& OutInvalidNodes) const
{
	EDataValidationResult Result = EDataValidationResult::Valid;
	
	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);
	
	for (UEdGraph* Graph: Graphs)
	{
		const uint32 NumErrors = Context.GetNumErrors();
		
		for (UEdGraphNode* Node: Graph->Nodes)
		{
			const EDataValidationResult NodeResult = ValidateNode(Blueprint, Node, AssetData,Context);
			if (NodeResult == EDataValidationResult::Invalid)
			{
				OutInvalidNodes.Add(Node);
			}
			Result &= NodeResult;
		}
		
		if (Context.GetNumErrors() > NumErrors)
		{
			Graph->NotifyGraphChanged();
		}
	}

	if (bValidateBlueprintVariables)
	{
		for (const FObjectPropertyBase* Property: TFieldRange<FObjectPropertyBase>{Blueprint->GeneratedClass, EFieldIterationFlags::None})
		{
			Result &= ValidateProperty(Blueprint, Property, AssetData, Context);
		}
	}

	return Result;
}

EDataValidationResult UAssetValidator_BlueprintGraph::ValidateProperty(const UBlueprint* Blueprint, const FProperty* Property, const FAssetData& AssetData, FDataValidationContext& Context) const
{
	using UE::AssetValidation::FBlueprintGraphToken;
	
	EDataValidationResult Result = EDataValidationResult::Valid;
	if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
	{
		const UClass* PropertyClass = ObjectProperty->PropertyClass;
		if (const FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
		{
			PropertyClass = ClassProperty->MetaClass;
		}
		check(PropertyClass);

		if (IsBlueprintGeneratedClass(PropertyClass))
		{
			UE::AssetValidation::AddTokenMessage(Context, EMessageSeverity::Error,
				AssetData, LOCTEXT("ValidateNode_BlueprintProperty", "Detected blueprint property with a type of a blueprint generated class.")
			);
			Result &= EDataValidationResult::Invalid;
		}
	}

	return Result;
}

EDataValidationResult UAssetValidator_BlueprintGraph::ValidateNode(const UBlueprint* Blueprint, UEdGraphNode* Node, const FAssetData& AssetData, FDataValidationContext& Context) const
{
	using UE::AssetValidation::FBlueprintGraphToken;
	EDataValidationResult Result = EDataValidationResult::Valid;
	
	if (bValidateBannedFunctions && !BannedFunctions.IsEmpty())
	{
		if (UK2Node_CallFunction* FunctionCall = Cast<UK2Node_CallFunction>(Node))
		{
			const FMemberReference& Reference = FunctionCall->FunctionReference;
			const UClass* MemberParent = Reference.GetMemberParentClass();
			const FName MemberName = Reference.GetMemberName();
			const FString FunctionName = FString::Printf(TEXT("%s.%s"), *GetNameSafe(MemberParent), *MemberName.ToString());

			for (const FString& BannedFunction: BannedFunctions)
			{
				if (BannedFunction == FunctionName)
				{
					const FText FailReason = FText::Format(LOCTEXT("ValidateNode_BannedFunction", "Function {0} is banned due to performance reasons."), FText::FromString(FunctionName));
					UE::AssetValidation::AddTokenMessage(Context, EMessageSeverity::Error,
						AssetData, FBlueprintGraphToken::Create(Node, Blueprint, FailReason), FailReason);
					
					HighlightNode(Node, FailReason);
					Result &= EDataValidationResult::Invalid;
					break;	
				}
			}
		}	
	}
	
	if (bValidateBlueprintCasts)
	{
		if (UK2Node_DynamicCast* DynamicCast = Cast<UK2Node_DynamicCast>(Node))
		{
			const UClass* TargetType = DynamicCast->TargetType;
			if (IsBlueprintGeneratedClass(TargetType))
			{
				const FText FailReason = LOCTEXT("ValidateNode_DynamicCast", "Dynamic cast to non-abstract blueprint type is prohibited because it creates a hard reference.");
				UE::AssetValidation::AddTokenMessage(Context, EMessageSeverity::Error, AssetData,
					 FBlueprintGraphToken::Create(Node, Blueprint, FailReason), FailReason);
				
				HighlightNode(Node, FailReason);
				Result &= EDataValidationResult::Invalid;
			}
		}
	}

	if (bValidateFunctionLocalVariables)
	{
		if (UK2Node_FunctionEntry* Function = Cast<UK2Node_FunctionEntry>(Node))
		{
			if (TSharedPtr<FStructOnScope> VariableCache = Function->GetFunctionVariableCache(); VariableCache.IsValid())
			{
				for (const FProperty* Property: TFieldRange<FObjectPropertyBase>{VariableCache->GetStruct(), EFieldIterationFlags::None})
				{
					Result &= ValidateProperty(Blueprint, Property, AssetData, Context);
				}
			}
		}
	}

	UClass* NodeClass = Node->GetClass();
	if (bValidateFunctionPins && BannedFunctionPins.Contains(NodeClass))
	{
		for (const UEdGraphPin* Pin: Node->Pins)
		{
			if (ValidatePin(Pin) == false)
			{
				const FText FailReason = LOCTEXT("ValidateNode_BPFunctionArgument", "Pin of an blueprint type is prohibited because it creates a hard reference.");
				UE::AssetValidation::AddTokenMessage(Context, EMessageSeverity::Error, AssetData,
					FBlueprintGraphToken::Create(Pin, Blueprint, FailReason), FailReason);
				
				HighlightNode(Node, FailReason);
				Result &= EDataValidationResult::Invalid;
			}
		}
	}


	return Result;
}

bool UAssetValidator_BlueprintGraph::ValidatePin(const UEdGraphPin* Pin)
{
	if (Pin == nullptr)
	{
		return false;
	}

	static TArray<FName, TInlineAllocator<4>> Names{UEdGraphSchema_K2::PC_Class, UEdGraphSchema_K2::PC_Object, UEdGraphSchema_K2::PC_SoftClass, UEdGraphSchema_K2::PC_SoftObject};
	if (Algo::Find(Names, Pin->PinType.PinCategory))
	{
		if (const UClass* TargetType = Cast<UClass>(Pin->PinType.PinSubCategoryObject.Get()))
		{
			return !IsBlueprintGeneratedClass(TargetType);
		}
	}

	return true;
}

bool UAssetValidator_BlueprintGraph::IsBlueprintGeneratedClass(const UClass* Class)
{
	return Class && Cast<UBlueprintGeneratedClass>(Class) && !Class->HasAnyClassFlags(CLASS_Abstract);
}

void UAssetValidator_BlueprintGraph::HighlightNode(const UEdGraphNode* Node, const FText& Msg, bool bNotifyGraphUpdated)
{
	check(Node);
	
	UEdGraphNode* MutableNode = const_cast<UEdGraphNode*>(Node);
	MutableNode->bHasCompilerMessage = true;
    MutableNode->ErrorType = EMessageSeverity::Error;
    MutableNode->ErrorMsg = Msg.ToString();

	if (bNotifyGraphUpdated)
	{
		UEdGraph* Graph = Node->GetGraph();
		check(Graph);
		
		Graph->NotifyGraphChanged();
	}
}

#undef LOCTEXT_NAMESPACE
