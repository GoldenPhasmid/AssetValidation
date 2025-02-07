#pragma once

#include "CoreMinimal.h"
#include "AVCommandletAssetSearchFilter.h"

#include "AVCommandletSearchFilter_Blueprints.generated.h"

UCLASS(DisplayName = "Blueprints", HideCategories = ("Filter|Maps", "Filter|Assets"))
class UAVCommandletSearchFilter_Blueprints: public UAVCommandletAssetSearchFilter
{
	GENERATED_BODY()
public:
	UAVCommandletSearchFilter_Blueprints();

	virtual void AddClassPaths(TArray<FTopLevelAssetPath>& ClassPaths) const override;
};
