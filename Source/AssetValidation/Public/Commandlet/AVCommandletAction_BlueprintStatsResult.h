#pragma once

#include "CoreMinimal.h"
#include "AVCommandletAction.h"

#include "AVCommandletAction_BlueprintStatsResult.generated.h"

USTRUCT()
struct FAVCommandletAction_BlueprintStatsResult: public FAVCommandletActionResultBase
{
	GENERATED_BODY()

	FAVCommandletAction_BlueprintStatsResult() = default;
	explicit FAVCommandletAction_BlueprintStatsResult(const UBlueprint* Blueprint, const FAssetData& AssetData);
	
	UPROPERTY()
	int32 Graphs = 0;
	
	UPROPERTY()
	int32 Events = 0;

	UPROPERTY()
	int32 MacroGraphs = 0;
	
	UPROPERTY()
	int32 Functions = 0;

	UPROPERTY()
	int32 Components = 0;

	UPROPERTY()
	int32 Timelines = 0;

	UPROPERTY()
	int32 Variables = 0;
	
	UPROPERTY()
	int32 Nodes = 0;
	
	UPROPERTY()
	int32 Comments = 0;
};

UCLASS(DisplayName = "Blueprint Stats")
class ASSETVALIDATION_API UAVCommandletAction_BlueprintStats: public UAVCommandletAction
{
	GENERATED_BODY()
public:
	virtual bool Run(const TArray<FAssetData>& InAssets) override;

	FAVCommandletAction_BlueprintStatsResult GetBlueprintStats(const UBlueprint* Blueprint, const FAssetData& Asset) const;
	
	UPROPERTY(EditAnywhere, meta = (EditCondition = "OutputFormat == EOutputFormat::Csv", FilePathFilter = "Csv File (*.csv)|*.csv"))
	FFilePath OutFile;
};
