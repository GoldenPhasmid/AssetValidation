#pragma once

#include "CoreMinimal.h"
#include "AVCommandletSearchFilter.h"

#include "AVCommandletAssetSearchFilter.generated.h"

UCLASS(DisplayName = "Asset Search Filter")
class ASSETVALIDATION_API UAVCommandletAssetSearchFilter: public UAVCommandletSearchFilter
{
	GENERATED_BODY()
public:

	//~Begin CommandletSearchFilter interface
	virtual void InitFromCommandlet(const TArray<FString>& Switches, const TMap<FString, FString>& Params) override;
	virtual bool GetAssets(TArray<FAssetData>& OutAssets) const override;
	//~End CommandletSearchFilter interface
	
	TArray<FTopLevelAssetPath> GetAllowedClasses() const;
	TArray<FName> GetPackagePaths() const;

	// interface to implement
	virtual void FilterAssets(TArray<FAssetData>& Assets) const {}
	virtual void AddPackagePaths(TArray<FName>& Paths) const {}
	virtual void AddClassPaths(TArray<FTopLevelAssetPath>& ClassPaths) const {}

	/** Specify a list of package paths to include into search request */
	UPROPERTY(EditAnywhere, Category = "Filter", meta = (Validate, ContentDir))
	TArray<FDirectoryPath> DirectoryPaths;
	
	/** Specify a list of asset types to include into search request */
	UPROPERTY(EditAnywhere, Category = "Filter", meta = (Validate, AllowAbstract = "true"))
	TArray<const UClass*> AssetTypes;

	UPROPERTY(EditAnywhere, Category = "Filter")
	bool bAllAssetTypes = false;
	
	UPROPERTY(EditAnywhere, Category = "Filter")
	bool bBlueprints = false;

	UPROPERTY(EditAnywhere, Category = "Filter")
	bool bWidgets = false;

	UPROPERTY(EditAnywhere, Category = "Filter")
	bool bAnimations = false;

	UPROPERTY(EditAnywhere, Category = "Filter")
	bool bSounds = false;

	/** Specify a list of maps to search for, exclusive with @AllMaps. */
	UPROPERTY(EditAnywhere, Category = "Filter", meta = (Validate, EditCondition = "!bAllMaps"))
	TArray<TSoftObjectPtr<UWorld>> Maps;

	/** will include all maps into search request, exclusive with @Maps */
	UPROPERTY(EditAnywhere, Category = "Filter")
	bool bAllMaps = false;

	UPROPERTY()
	TArray<FName> CommandletPackagePaths;
	
	UPROPERTY()
	TArray<FName> CommandletMapNames;
};
