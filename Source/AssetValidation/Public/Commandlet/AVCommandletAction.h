#pragma once

#include "CoreMinimal.h"

#include "AVCommandletAction.generated.h"

struct FAssetData;

UCLASS(Abstract, EditInlineNew, DefaultToInstanced)
class ASSETVALIDATION_API UAVCommandletAction: public UObject
{
	GENERATED_BODY()
public:

	/** */
	virtual void InitFromCommandlet(const TArray<FString>& Switches, const TMap<FString, FString>& Params) {}

	/** */
	virtual bool Run(const TArray<FAssetData>& Assets) { return false; }
	
};
