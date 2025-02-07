#pragma once

#include "CoreMinimal.h"
#include "AVCommandletSearchFilter.h"

#include "AVCommandletAssetSearchFilter.generated.h"

/**
 * Asset Search Filter
 */
UCLASS(DisplayName = "Asset Search Filter")
class ASSETVALIDATION_API UAVCommandletAssetSearchFilter: public UAVCommandletSearchFilter
{
	GENERATED_BODY()
public:
	UAVCommandletAssetSearchFilter();

	//~Begin CommandletSearchFilter interface
	virtual void PostInitProperties() override;
	virtual void InitFromCommandlet(const TArray<FString>& Switches, const TMap<FString, FString>& Params) override;
	virtual bool GetAssets(TArray<FAssetData>& OutAssets) const override;
	//~End CommandletSearchFilter interface
	
	TArray<FTopLevelAssetPath> GetAllowedClasses() const;
	TSet<FTopLevelAssetPath> GetExcludedClasses() const;
	TArray<FName> GetPackagePaths() const;

protected:

	/** add classes derived from @ParentClass to @ClassPaths array */
	static void AddDerivedClasses(TArray<FTopLevelAssetPath>& ClassPaths, UClass* ParentClass);
	/** post process search filter result */
	virtual void PostProcessAssets(TArray<FAssetData>& Assets) const {}
	/** append additional package paths to the AR filter */
	virtual void AddPackagePaths(TArray<FName>& Paths) const {}
	/** append additional class paths to the AR filter */
	virtual void AddClassPaths(TArray<FTopLevelAssetPath>& ClassPaths) const {}

	/** Specify a list of package paths to include into search request */
	UPROPERTY(EditAnywhere, Category = "Filter|Paths", meta = (Validate, ContentDir))
	TArray<FDirectoryPath> DirectoryPaths;

	/** If true, assets in @DirectoryPaths discovered recursively */
	UPROPERTY(EditAnywhere, Category = "Filter|Paths")
	bool bRecursivePaths = true;

	/** will include all maps into search request, exclusive with @Maps */
	UPROPERTY(EditAnywhere, Category = "Filter|Maps")
	bool bAllMaps = false;

	/** Specify a list of maps to search for, exclusive with @AllMaps. */
	UPROPERTY(EditAnywhere, Category = "Filter|Maps", meta = (Validate, EditCondition = "!bAllMaps"))
	TArray<TSoftObjectPtr<UWorld>> Maps;

	UPROPERTY(EditAnywhere, Category = "Filter|Assets")
	bool bAllAssetTypes = false;

	UPROPERTY(EditAnywhere, Category = "Filter|Assets")
	bool bRecursiveTypes = false;

	/** Specify a list of asset types to include into search request */
	UPROPERTY(EditAnywhere, Category = "Filter|Assets", meta = (Validate, EditCondition = "!bAllAssetTypes", AllowAbstract = "true"))
	TArray<const UClass*> AssetTypes;

	UPROPERTY(EditAnywhere, Category = "Filter|Assets", meta = (Validate, AllowAbstract = "true"))
	TArray<const UClass*> ExcludeAssetTypes;
	
	UPROPERTY(EditAnywhere, Category = "Filter|Assets|Quick", meta = (EditCondition = "!bAllAssetTypes"))
	uint32 bBlueprints: 1 = false;

	UPROPERTY(EditAnywhere, Category = "Filter|Assets|Quick", meta = (EditCondition = "!bAllAssetTypes"))
	uint32 bWidgets = false;
	
	UPROPERTY(EditAnywhere, Category = "Filter|Assets|Quick", meta = (EditCondition = "!bAllAssetTypes"))
	uint32 bEditorUtilityWidgets: 1 = false;

	UPROPERTY(EditAnywhere, Category = "Filter|Assets|Quick", meta = (EditCondition = "!bAllAssetTypes"))
	uint32 bAnimations: 1 = false;

	UPROPERTY(EditAnywhere, Category = "Filter|Assets|Quick", meta = (EditCondition = "!bAllAssetTypes"))
	uint32 bStaticMeshes: 1 = false;

	UPROPERTY(EditAnywhere, Category = "Filter|Assets|Quick", meta = (EditCondition = "!bAllAssetTypes"))
	uint32 bSkeletalMeshes: 1 = false;

	UPROPERTY(EditAnywhere, Category = "Filter|Assets|Quick", meta = (EditCondition = "!bAllAssetTypes"))
	uint32 bMaterials: 1 = false;

	UPROPERTY(EditAnywhere, Category = "Filter|Assets|Quick", meta = (EditCondition = "!bAllAssetTypes"))
	uint32 bTextures: 1 = false;

	UPROPERTY(EditAnywhere, Category = "Filter|Assets|Quick", meta = (EditCondition = "!bAllAssetTypes"))
	uint32 bSounds: 1 = false;
	
	UPROPERTY(EditAnywhere, Category = "Filter|Assets|Quick", meta = (EditCondition = "!bAllAssetTypes"))
	uint32 bRedirectors: 1 = false;
	
	UPROPERTY()
	TArray<FName> CommandletPackagePaths;
	
	UPROPERTY()
	TArray<FName> CommandletMapNames;
};
