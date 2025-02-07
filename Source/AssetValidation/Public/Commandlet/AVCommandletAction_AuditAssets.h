#pragma once

#include "CoreMinimal.h"
#include "AVCommandletAction.h"

#include "AVCommandletAction_AuditAssets.generated.h"

struct FAssetData;
struct FAssetAuditResult;
class IAssetRegistry;
class IAssetManagerEditorModule;

USTRUCT()
struct FAVCommandletAction_AuditAssetResult: public FAVCommandletActionResultBase
{
	GENERATED_BODY()
	
	FAVCommandletAction_AuditAssetResult() = default;
	explicit FAVCommandletAction_AuditAssetResult(const FAssetAuditResult& InResult);

	UPROPERTY(DisplayName = "Disk Size (MB")
	float DiskSizeMB = 0.f;

	UPROPERTY(DisplayName = "Memory Size (MB")
	float MemorySizeMB = 0.f;

	UPROPERTY()
	int32 DependencyCount = 0;

	UPROPERTY()
	int32 DependencyChainDepth = 0;
};

UCLASS(DisplayName = "Audit Assets")
class ASSETVALIDATION_API UAVCommandletAction_AuditAssets: public UAVCommandletAction
{
	GENERATED_BODY()
public:

	virtual void InitFromCommandlet(const TArray<FString>& Switches, const TMap<FString, FString>& Params) override;
	virtual bool Run(const TArray<FAssetData>& InAssets) override;
	
	UPROPERTY(EditAnywhere)
	bool bDisplaySummary = false;
	
	UPROPERTY(EditAnywhere, meta = (EditCondition = "OutputFormat == EOutputFormat::Csv", FilePathFilter = "Csv File (*.csv)|*.csv"))
	FFilePath OutFile;
};
