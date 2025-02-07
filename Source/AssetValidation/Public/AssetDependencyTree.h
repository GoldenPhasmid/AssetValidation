#pragma once

#include "CoreMinimal.h"

class IAssetRegistry;
struct FAssetData;

/**
 * AssetTreeNode
 */
struct ASSETVALIDATION_API FAssetTreeNode
{
	FAssetTreeNode() = default;
	FAssetTreeNode(IAssetRegistry& AssetRegistry, const FAssetData& InAssetData);

	bool IsValid() const;

	FAssetData AssetData;
	float TotalDiskSizeBytes = 0.f;
	float TotalMemorySizeBytes = 0.f;
	
	TArray<TSharedPtr<FAssetTreeNode>> Children;
};

/**
 * AssetAuditResult
 */
struct ASSETVALIDATION_API FAssetAuditResult
{
	FAssetAuditResult() = default;
	explicit FAssetAuditResult(const FAssetData& InAssetData);
	
	FAssetData AssetData;
	float TotalDiskSizeBytes = 0.f;
	float TotalMemorySizeBytes = 0.f;
	int32 TotalDependencyCount = 0;
	int32 MaxDependencyBreadth = 0;
	int32 DependencyDepth = 0;

	FORCEINLINE void Add(const TSharedPtr<FAssetTreeNode>& AssetNode)
	{
		TotalDiskSizeBytes += AssetNode->TotalDiskSizeBytes;
		TotalMemorySizeBytes += AssetNode->TotalMemorySizeBytes;
	}
};

/**
 * Asset Dependency Tree
 * Provides general information about asset and its HARD references
 * Unlike AssetRegistry, dependencies are stored as AssetData, which makes subsequent queries faster
 * The main reason though is that it handles recursive dependency iteration for you
 * @todo: tree is not usable multiple times, it gives invalid results
 * @todo: thread safety for adding/removing nodes
 * @todo: asset data invalidation
 * @todo: cache referencers as well
 * @todo: make it a dependency graph instead
 */
class ASSETVALIDATION_API FAssetDependencyTree
{
public:
	~FAssetDependencyTree();
	
	/**
	 * Produce an asset audit result by recursively iterating over asset dependencies. Works similar to Size Map, only worse
	 * @return true if operation was successful
	 */
	bool AuditAsset(IAssetRegistry& AssetRegistry, const FAssetData& Asset, FAssetAuditResult& OutResult);
	
protected:

	void Reset();
	
	static TSharedPtr<FAssetTreeNode> CreateAssetNode(IAssetRegistry& AssetRegistry, const FAssetData& AssetData, FAssetDependencyTree& DependencyTree, TSet<FName>& VisitedAssets);
	static void AuditAsset(const TSharedPtr<FAssetTreeNode>& TreeNode, FAssetAuditResult& OutResult, int32 CurrentDepth = 1);

	TMap<FAssetIdentifier, TSharedPtr<FAssetTreeNode>> TreeNodes;
};
