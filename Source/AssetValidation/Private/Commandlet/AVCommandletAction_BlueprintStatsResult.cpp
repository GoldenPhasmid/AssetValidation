#include "Commandlet/AVCommandletAction_BlueprintStatsResult.h"

#include "AssetValidationSettings.h"
#include "EdGraphNode_Comment.h"
#include "Commandlet/AVCommandletUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

FAVCommandletAction_BlueprintStatsResult::FAVCommandletAction_BlueprintStatsResult(const UBlueprint* Blueprint, const FAssetData& AssetData)
	: Super(AssetData)
{
	Graphs		= Blueprint->UbergraphPages.Num();
	Events		= Blueprint->EventGraphs.Num();
	MacroGraphs = Blueprint->MacroGraphs.Num();
	Functions	= Blueprint->FunctionGraphs.Num();
	Components	= Blueprint->ComponentTemplates.Num();
	Timelines	= Blueprint->Timelines.Num();
	Variables	= Blueprint->NewVariables.Num();
}

bool UAVCommandletAction_BlueprintStats::Run(const TArray<FAssetData>& InAssets)
{
	if (InAssets.IsEmpty())
	{
		return true;
	}

	const int32 NumAssets = InAssets.Num();
	FScopedSlowTask SlowTask(NumAssets, LOCTEXT("UAVCommandletAction_BlueprintNodeCountTask", "Processing Blueprints..."));
	SlowTask.MakeDialog(NumAssets > UAssetValidationSettings::Get()->NumAssetsToShowCancelButton);

	TArray<FAVCommandletAction_BlueprintStatsResult> Results;
	Results.Reserve(NumAssets);
	
	for (const FAssetData& Asset: InAssets)
	{
		if (SlowTask.ShouldCancel())
		{
			break;
		}
		SlowTask.EnterProgressFrame(1.0f);
		
		UBlueprint* Blueprint = Cast<UBlueprint>(Asset.GetAsset({}));
		if (Blueprint == nullptr)
		{
			continue;
		}
		
		Results.Add(GetBlueprintStats(Blueprint, Asset));
	}
	
	const FString ExportText = UE::AssetValidation::CsvExport(Results);
	const bool bResult = FFileHelper::SaveStringToFile(ExportText, *FPaths::ConvertRelativePathToFull(OutFile.FilePath));
	
	return bResult;
}

FAVCommandletAction_BlueprintStatsResult UAVCommandletAction_BlueprintStats::GetBlueprintStats(const UBlueprint* Blueprint, const FAssetData& Asset) const
{
	FAVCommandletAction_BlueprintStatsResult Result{Blueprint, Asset};
	if (FBlueprintEditorUtils::IsDataOnlyBlueprint(Blueprint))
	{
		return Result;
	}

	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);
	
	for (UEdGraph* Graph: Graphs)
	{
		Result.Nodes += Graph->Nodes.Num();
		for (UEdGraphNode* Node: Graph->Nodes)
		{
			Result.Comments += Node->IsA<UEdGraphNode_Comment>();
		}
	}
	
	return Result;
}

#undef LOCTEXT_NAMESPACE