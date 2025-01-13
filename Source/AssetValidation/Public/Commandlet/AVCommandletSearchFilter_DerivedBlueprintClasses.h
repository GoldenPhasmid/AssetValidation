#pragma once

#include "CoreMinimal.h"
#include "AVCommandletAssetSearchFilter.h"

#include "AVCommandletSearchFilter_DerivedBlueprintClasses.generated.h"

UCLASS(meta = (DisplayName = "Derived Blueprint Classes"))
class UAVCommandletSearchFilter_DerivedBlueprintClasses: public UAVCommandletSearchFilter
{
	GENERATED_BODY()
public:
	UAVCommandletSearchFilter_DerivedBlueprintClasses();
	
	//~Begin CommandletSearchFilter interface
	virtual void PostInitProperties() override;
	virtual void InitFromCommandlet(const TArray<FString>& Switches, const TMap<FString, FString>& Params) override;
	virtual bool GetAssets(TArray<FAssetData>& OutAssets) const override;
	//~End CommandletSearchFilter interface

	UPROPERTY(EditAnywhere, Category = "Filter")
	UClass* BaseClass = nullptr;

	/** Specify a list of package paths to include into search request. Empty means ALL */
	UPROPERTY(EditAnywhere, Category = "Filter", meta = (Validate, ContentDir))
	TArray<FDirectoryPath> Paths;
};
