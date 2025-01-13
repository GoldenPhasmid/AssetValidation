#include "AssetDependencyTree.h"

#include "AssetValidationStatics.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Misc/AssetRegistryInterface.h"

FAssetAuditResult::FAssetAuditResult(const FAssetData& InAssetData)
	: AssetData(InAssetData)
{
	
}

FAssetTreeNode::FAssetTreeNode(IAssetRegistry& AssetRegistry, const FAssetData& InAssetData)
	: AssetData(InAssetData)
{
	UE::AssetValidation::GetAssetSizeBytes(AssetRegistry, AssetData, TotalMemorySizeBytes, TotalDiskSizeBytes);
}

bool FAssetTreeNode::IsValid() const
{
	return AssetData.IsValid() && !FMath::IsNearlyZero(TotalDiskSizeBytes) && !FMath::IsNearlyZero(TotalMemorySizeBytes);
}

FAssetDependencyTree::~FAssetDependencyTree()
{
	Reset();
}

bool FAssetDependencyTree::AuditAsset(IAssetRegistry& AssetRegistry, const FAssetData& Asset, FAssetAuditResult& OutResult)
{
	TSet<FName> VisitedAssets{};
	TSharedPtr<FAssetTreeNode> TreeNode = CreateAssetNode(AssetRegistry, Asset, *this, VisitedAssets);
	if (!TreeNode.IsValid())
	{
		return false;
	}

	constexpr int32 StartDepth = 1;
	AuditAsset(TreeNode, OutResult, StartDepth);

	return true;
}

void FAssetDependencyTree::Reset()
{
	TreeNodes.Reset();
}

TSharedPtr<FAssetTreeNode> FAssetDependencyTree::CreateAssetNode(IAssetRegistry& AssetRegistry, const FAssetData& AssetData, FAssetDependencyTree& DependencyTree, TSet<FName>& VisitedAssets)
{
	check(AssetData.IsValid());
	check(!VisitedAssets.Contains(AssetData.PackageName));
	VisitedAssets.Add(AssetData.PackageName);
	
	FAssetIdentifier AssetId{AssetData.PackageName};
	if (TSharedPtr<FAssetTreeNode>* AssetNode = DependencyTree.TreeNodes.Find(AssetId))
	{
		return *AssetNode;
	}

	// @todo: large block allocations instead of shared references
	TSharedPtr<FAssetTreeNode> AssetNode = DependencyTree.TreeNodes.Add(AssetId, MakeShared<FAssetTreeNode>(AssetRegistry, AssetData));

	TArray<FName> Dependencies;
	// @todo: not only hard dependencies
	AssetRegistry.GetDependencies(AssetData.PackageName, Dependencies, UE::AssetRegistry::EDependencyCategory::Package, UE::AssetRegistry::EDependencyQuery::Hard);
	
	for (FName Dependency: Dependencies)
	{
		FAssetIdentifier DependencyId{Dependency};
		const FString DependencyStr = Dependency.ToString();
		
		if (FPackageName::IsScriptPackage(DependencyStr))
		{
			// ignore script packages because they're not interesting
			continue;
		}

		if (VisitedAssets.Contains(Dependency))
		{
			// already visited dependency
			continue;
		}

		TSharedPtr<FAssetTreeNode> DependencyNode{};
		if (TSharedPtr<FAssetTreeNode>* DependencyNodePtr = DependencyTree.TreeNodes.Find(DependencyId))
		{
			// found existing node
			DependencyNode = *DependencyNodePtr;
		}
		else
		{
			const FString DependencyPath = FString::Printf(TEXT("%s.%s"), *DependencyStr, *FPackageName::GetLongPackageAssetName(DependencyStr));
			
			if (FAssetData DependencyAsset = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath{DependencyPath}); DependencyAsset.IsValid())
			{
				DependencyNode = CreateAssetNode(AssetRegistry, DependencyAsset, DependencyTree, VisitedAssets);
			}
			
		}
		if (DependencyNode.IsValid())
		{
			AssetNode->Children.Add(DependencyNode);
		}
	}

	return AssetNode;
}

void FAssetDependencyTree::AuditAsset(const TSharedPtr<FAssetTreeNode>& TreeNode, FAssetAuditResult& OutResult, int32 CurrentDepth)
{
	check(TreeNode.IsValid());
	
	OutResult.Add(TreeNode);

	OutResult.DependencyDepth = FMath::Max(OutResult.DependencyDepth, CurrentDepth);
	OutResult.MaxDependencyBreadth = FMath::Max(OutResult.MaxDependencyBreadth, TreeNode->Children.Num());
	OutResult.TotalDependencyCount += TreeNode->Children.Num();
	
	for (const TSharedPtr<FAssetTreeNode>& Child: TreeNode->Children)
	{
		AuditAsset(Child, OutResult, CurrentDepth + 1);
	}
}

