#pragma once

#include "CoreMinimal.h"

#include "AVCommandletSearchFilter.generated.h"

UCLASS(Abstract, EditInlineNew, DefaultToInstanced)
class ASSETVALIDATION_API UAVCommandletSearchFilter: public UObject
{
	GENERATED_BODY()
public:

	virtual void InitFromCommandlet(const TArray<FString>& Switches, const TMap<FString, FString>& Params) {}
	virtual bool GetAssets(TArray<FAssetData>& OutAssets) const { return false; }

	
};
