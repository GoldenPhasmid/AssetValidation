#include "Commandlet/AVCommandletAction_FindNodesAndExecuteAction.h"

#include "FileHelpers.h"
#include "AssetValidators/AssetValidator_BlueprintGraph.h"
#include "Misc/DataValidation.h"
#include "AssetValidationDefines.h"

namespace UE::AssetValidation
{
	static const FString CountNodes{("CountNodes")};
	static const FString DeleteNodes{("DeleteNodes")};
	static const FString RefreshNodes{TEXT("RefreshNodes")};
}


void UAVCommandletAction_FindNodesAndExecuteAction::PostInitProperties()
{
	Super::PostInitProperties();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		Validator = NewObject<UAssetValidator_BlueprintGraph>(this);
	}
}

void UAVCommandletAction_FindNodesAndExecuteAction::InitFromCommandlet(const TArray<FString>& Switches, const TMap<FString, FString>& Params)
{
	Super::InitFromCommandlet(Switches, Params);

	check(Validator);

	if (Switches.Contains(UE::AssetValidation::CountNodes))
	{
		NodeAction = ENodeAction::Count;
	}
	else if (Switches.Contains(UE::AssetValidation::RefreshNodes))
	{
		NodeAction = ENodeAction::Refresh;
	}
	else if (Switches.Contains(UE::AssetValidation::DeleteNodes))
	{
		NodeAction = ENodeAction::Delete;
	}
}

bool UAVCommandletAction_FindNodesAndExecuteAction::Run(const TArray<FAssetData>& Assets)
{
	if (Assets.IsEmpty())
	{
		return true;
	}
	
	check(Validator);
	FDataValidationContext Context;

	TArray<UPackage*> Packages;
	TMap<const UBlueprint*, TArray<UEdGraphNode*>> NodeMap;
	
	for (const FAssetData& AssetData: Assets)
	{
		UObject* Asset = AssetData.GetAsset();
		if (Validator->CanValidateAsset_Implementation(AssetData, Asset, Context))
		{
			const UBlueprint* Blueprint = CastChecked<UBlueprint>(Asset);

			TArray<UEdGraphNode*> InvalidNodes;
			const EDataValidationResult Result = Validator->ValidateBlueprint(Blueprint, AssetData, Context, InvalidNodes);

			if (Result == EDataValidationResult::Invalid)
			{
				Packages.Add(AssetData.GetPackage());
				NodeMap.Add(Blueprint, MoveTemp(InvalidNodes));
			}
		}
	}

	if (!NodeMap.IsEmpty())
	{
		int32 TotalBlueprints = 0;
		int32 TotalNodes = 0;
		for (auto& [Blueprint, Nodes]: NodeMap)
		{
			if (NodeAction == ENodeAction::Count)
			{
				++TotalBlueprints;
				TotalNodes += Nodes.Num();
				UE_LOG(LogAssetValidation, Display, TEXT("Found %d invalid nodes for blueprint %s."), Nodes.Num(), *Blueprint->GetName());
			}
			else if (NodeAction == ENodeAction::Refresh)
			{
				// @todo: implement
			}
			else if (NodeAction == ENodeAction::Delete)
			{
				for (UEdGraphNode* InvalidNode: Nodes)
				{
					UEdGraph* Graph = InvalidNode->GetGraph();
					Graph->RemoveNode(InvalidNode);
				}
			}
		}

		if (NodeAction == ENodeAction::Count)
		{
			UE_LOG(LogAssetValidation, Display, TEXT("Found total %d blueprints with %d invalid nodes."), TotalBlueprints, TotalNodes);
		}
	}

	if (NodeAction == ENodeAction::Refresh || NodeAction == ENodeAction::Delete)
	{
		for (const UPackage* Package: Packages)
		{
			(void)Package->MarkPackageDirty();
		}
		
		UE_LOG(LogAssetValidation, Display, TEXT("Saving %d packages."), Packages.Num());
        FEditorFileUtils::PromptForCheckoutAndSave(Packages, false, false);
	}

	return true;
}
