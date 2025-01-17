#include "Commandlet/AVCommandletAction_MigrateBlueprintComponent.h"

#include "K2Node_CallFunction.h"
#include "K2Node_Variable.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Kismet2/BlueprintEditorUtils.h"

bool UAVCommandletAction_MigrateBlueprintComponent::Run(const TArray<FAssetData>& Assets)
{
	return Super::Run(Assets);
}

bool UAVCommandletAction_MigrateBlueprintComponent::CopyComponentProperties(UBlueprint* Blueprint, FName OldComponentName, FName NewComponentName)
{
	if (Blueprint == nullptr || OldComponentName == NAME_None || NewComponentName == NAME_None)
	{
		return false;
	}

	UClass* Class = Cast<UClass>(Blueprint->GeneratedClass);
	
	UClass* NativeClass = Blueprint->ParentClass;
	while (NativeClass && Cast<UBlueprintGeneratedClass>(NativeClass))
	{
		NativeClass = NativeClass->GetSuperClass();
	}
	
	if (const UActorComponent* OldComponent = AActor::GetActorClassDefaultComponentByName(Class, UActorComponent::StaticClass(), OldComponentName))
	{
		if (const UActorComponent* NewComponent = AActor::GetActorClassDefaultComponentByName(Class, UActorComponent::StaticClass(), NewComponentName))
		{
			if (OldComponent == NewComponent)
			{
				// components are the same
				return false;
			}

			UActorComponent* MutableOldComponent = const_cast<UActorComponent*>(OldComponent);
			UActorComponent* MutableNewComponent = const_cast<UActorComponent*>(NewComponent);
			
			UClass* OldComponentClass = OldComponent->GetClass();
			UClass* NewComponentClass = NewComponent->GetClass();
			
			Blueprint->Modify();
			
			FProperty* OldProperty = Class->FindPropertyByName(OldComponentName);
			FProperty* NewProperty = Class->FindPropertyByName(NewComponentName);
			check(OldProperty && NewProperty);

			// copy properties from old to new component
			UEngine::FCopyPropertiesForUnrelatedObjectsParams Params{};
			Params.bDoDelta = false;
			
			GEngine->CopyPropertiesForUnrelatedObjects(MutableOldComponent, MutableNewComponent, Params);
			
			// try to replace references to old component from specified graph nodes 
			{
				TArray<UEdGraph*> Graphs;
				Blueprint->GetAllGraphs(Graphs);
				
				for (UEdGraph* Graph: Graphs)
				{
					for (UEdGraphNode* Node: Graph->Nodes)
					{
						// replace variable references
						if (UK2Node_Variable* Variable = Cast<UK2Node_Variable>(Node); Variable && !Variable->IsIntermediateNode())
						{
							if (Variable->VariableReference.GetMemberName() == OldComponentName)
							{
								Variable->Modify();
							
								Variable->VariableReference.SetSelfMember(NewComponentName, FBlueprintEditorUtils::FindMemberVariableGuidByName(Blueprint, NewComponentName));
								if (UEdGraphPin* Pin = Variable->FindPin(OldComponentName, EGPD_Output))
								{
									Pin->Modify();
									Pin->PinName = NewComponentName;
									Pin->PinType.PinSubCategoryObject = NewComponent->GetClass();
								}
							}
						}
						// replace call functions
						else if (UK2Node_CallFunction* CallFunction = Cast<UK2Node_CallFunction>(Node))
						{
							if (UClass* MemberClass = CallFunction->FunctionReference.GetMemberParentClass(); MemberClass == OldComponentClass)
							{
								UFunction* NewFunction = NewComponentClass->FindFunctionByName(CallFunction->FunctionReference.GetMemberName());
								CallFunction->FunctionReference.SetFromField<UFunction>(NewFunction, false);
							}
						}
					}
				}
			}

			// Process attached blueprint components
			UBlueprintGeneratedClass::ForEachGeneratedClassInHierarchy(Class, [&](const UBlueprintGeneratedClass* CurrentBPGC)
			{
				if (const USimpleConstructionScript* const ConstructionScript = CurrentBPGC->SimpleConstructionScript)
				{
					// Gets all BP added components
					for (USCS_Node* Node : ConstructionScript->GetAllNodes())
					{
						if (Node->ParentComponentOrVariableName == OldComponentName)
						{
							Node->Modify();
							Node->ParentComponentOrVariableName = NewComponentName;
							Node->ParentComponentOwnerClassName = NAME_None;
							Node->bIsParentComponentNative = NewComponent->CreationMethod == EComponentCreationMethod::Native;
							if (!Node->bIsParentComponentNative)
							{
								Node->ParentComponentOwnerClassName = NewProperty->GetOwnerClass()->GetFName();
							}
							
							return false;
						}
					}
				}
				return true;
			});

			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
			return true;
		}
	}

	return false;
}
