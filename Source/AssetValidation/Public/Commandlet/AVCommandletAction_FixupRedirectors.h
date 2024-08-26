#pragma once

#include "CoreMinimal.h"
#include "AVCommandletAction.h"

#include "AVCommandletAction_FixupRedirectors.generated.h"

enum class ERedirectFixupMode;

UCLASS(DisplayName = "Fixup Redirectors")
class UAVCommandletAction_FixupRedirectors: public UAVCommandletAction
{
	GENERATED_BODY()
public:
	UAVCommandletAction_FixupRedirectors();
	
	virtual bool Run(const TArray<FAssetData>& Assets) override;

	UPROPERTY(EditAnywhere, Category = "Action")
	bool bPromptCheckout = false;

	UPROPERTY(EditAnywhere, Category = "Action")
	ERedirectFixupMode FixupMode;
};
