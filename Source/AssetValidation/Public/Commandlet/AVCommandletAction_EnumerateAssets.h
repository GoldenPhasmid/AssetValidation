#pragma once

#include "CoreMinimal.h"
#include "AVCommandletAction.h"

#include "AVCommandletAction_EnumerateAssets.generated.h"

UCLASS(DisplayName = "Enumerate Assets")
class UAVCommandletAction_EnumerateAssets: public UAVCommandletAction
{
	GENERATED_BODY()
public:

	virtual bool Run(const TArray<FAssetData>& Assets) override;

	UPROPERTY(EditAnywhere)
	bool bLogAssets = false;
};
