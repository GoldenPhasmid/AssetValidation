#pragma once

#include "CoreMinimal.h"
#include "AVCommandletAction.h"

#include "AVCommandletAction_AuditAssets.generated.h"

struct FAssetData;
class IAssetRegistry;
class IAssetManagerEditorModule;


UCLASS(DisplayName = "Audit Assets")
class ASSETVALIDATION_API UAVCommandletAction_AuditAssets: public UAVCommandletAction
{
	GENERATED_BODY()
public:

	virtual void InitFromCommandlet(const TArray<FString>& Switches, const TMap<FString, FString>& Params) override;
	virtual bool Run(const TArray<FAssetData>& Assets) override;
	
	UPROPERTY(EditAnywhere)
	FString OutputFile;

	UPROPERTY(EditAnywhere)
	int32 MaxCount = 0;
	
	UPROPERTY(EditAnywhere)
	bool bDisplaySummary = false;
};
