#include "Commandlet/AVCommandletAction_AuditAssets.h"

#include "AssetValidationDefines.h"
#include "Algo/IndexOf.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Misc/EnumerateRange.h"

namespace UE::AssetValidation
{
	static const FString DisplaySummary{TEXT("Summary")};
	static const FString MaxCount{TEXT("MaxCount")};
}

FAssetTreeNode::FAssetTreeNode(IAssetRegistry& AssetRegistry, const FAssetData& InAssetData)
	: AssetData(InAssetData)
{
	if (TOptional<FAssetPackageData> FoundData = AssetRegistry.GetAssetPackageDataCopy(AssetData.PackageName); FoundData.IsSet())
	{
		TotalDiskSizeBytes = FoundData->DiskSize;
	}

	if (UObject* Asset = AssetData.GetAsset())
	{
		TotalMemorySizeBytes = Asset->GetResourceSizeBytes(EResourceSizeMode::EstimatedTotal);
	}
}

bool FAssetTreeNode::IsValid() const
{
	return AssetData.IsValid() && !FMath::IsNearlyZero(TotalDiskSizeBytes) && !FMath::IsNearlyZero(TotalMemorySizeBytes);
}

void UAVCommandletAction_AuditAssets::InitFromCommandlet(const TArray<FString>& Switches, const TMap<FString, FString>& Params)
{
	bDisplaySummary = Switches.Contains(UE::AssetValidation::DisplaySummary);
	if (const FString* Value = Params.Find(UE::AssetValidation::MaxCount))
	{
		MaxCount = FCString::Atoi(**Value);
	}
}

bool UAVCommandletAction_AuditAssets::Run(const TArray<FAssetData>& Assets)
{
	if (Assets.IsEmpty())
	{
		return true;
	}
	
 	IAssetRegistry& AssetRegistry = IAssetRegistry::GetChecked();
	const int32 NumAssets = Assets.Num();
	
	TArray<TSharedPtr<FAssetTreeNode>> AssetNodes;
	AssetNodes.Reserve(NumAssets);
	
	FAssetDependencyTree DependencyTree{};
	for (TEnumerateRef<const FAssetData> AssetData: EnumerateRange(Assets))
	{
		TSet<FName> VisitedAssets{};
		UE_LOG(LogAssetValidation, Display, TEXT("creating asset node %d/%d"), AssetData.GetIndex(), NumAssets);
		AssetNodes.Add(CreateAssetNode(AssetRegistry, *AssetData, DependencyTree, VisitedAssets));
	}

	TArray<FAssetAuditResult> Results;
	Results.Reserve(NumAssets);
	
	for (const TSharedPtr<FAssetTreeNode>& AssetTreeNode: AssetNodes)
	{
		FAssetAuditResult& Result = Results.AddDefaulted_GetRef();
		AuditAsset(AssetTreeNode, Result);
	}

	Algo::Sort(Results, [](const FAssetAuditResult& Lhs, const FAssetAuditResult& Rhs)
	{
		return Lhs.TotalMemorySizeBytes < Rhs.TotalMemorySizeBytes ||
				FMath::IsNearlyEqual(Lhs.TotalMemorySizeBytes, Rhs.TotalMemorySizeBytes) && Lhs.TotalDiskSizeBytes < Rhs.TotalDiskSizeBytes || 
				FMath::IsNearlyEqual(Lhs.TotalMemorySizeBytes, Rhs.TotalMemorySizeBytes) && FMath::IsNearlyEqual(Lhs.TotalDiskSizeBytes, Rhs.TotalDiskSizeBytes) && Lhs.TotalDependencyCount < Rhs.TotalDependencyCount;
	});
	
	constexpr int32 GigaByte = 1024 * 1024 * 1024;
	constexpr int32 KiloByte = 1024;
	const int32 GigaMemorySize = Algo::IndexOfByPredicate(Results, [GigaByte](const FAssetAuditResult& Result) -> bool { return Result.TotalMemorySizeBytes / GigaByte > 1.0; });
	const int32 GigaDiskSize = Algo::IndexOfByPredicate(Results, [GigaByte](const FAssetAuditResult& Result) -> bool { return Result.TotalDiskSizeBytes / GigaByte > 1.0; });

	if (bDisplaySummary)
	{
		const FAssetAuditResult& Result = Results[Results.Num() / 2];

		UE_LOG(LogAssetValidation, Display, TEXT("AuditAssets - Processed %d assets."), NumAssets);
		UE_LOG(LogAssetValidation, Display, TEXT("AuditAssets - Median Dependency Count: %d"), Result.TotalDependencyCount);
		UE_LOG(LogAssetValidation, Display, TEXT("AuditAssets - Median Memory Size: %.2fKB"), Result.TotalMemorySizeBytes / KiloByte);
		UE_LOG(LogAssetValidation, Display, TEXT("AuditAssets - Median Disk Size: %.2fKB"), Result.TotalDiskSizeBytes / KiloByte);
		UE_LOG(LogAssetValidation, Display, TEXT("AuditAssets - Median Depth: %d"), Result.DependencyDepth);
		UE_LOG(LogAssetValidation, Display, TEXT("AuditAssets - Median Breadth: %d"), Result.MaxDependencyBreadth);
		UE_LOG(LogAssetValidation, Display, TEXT("AuditAssets - Num assets > 1 GB in memory size: %d"), NumAssets - GigaMemorySize + 1);
		UE_LOG(LogAssetValidation, Display, TEXT("AuditAssets - Num assets > 1 GB in disk size: %d"), NumAssets - GigaDiskSize + 1);
	}

	return true;
}

TSharedPtr<FAssetTreeNode> UAVCommandletAction_AuditAssets::CreateAssetNode(IAssetRegistry& AssetRegistry, const FAssetData& AssetData, FAssetDependencyTree& DependencyTree, TSet<FName>& VisitedAssets) const
{
	check(AssetData.IsValid());
	check(!VisitedAssets.Contains(AssetData.PackageName));
	VisitedAssets.Add(AssetData.PackageName);
	
	FAssetIdentifier AssetId{AssetData.PackageName};
	if (TSharedPtr<FAssetTreeNode>* AssetNode = DependencyTree.Find(AssetId))
	{
		return *AssetNode;
	}
	
	TSharedPtr<FAssetTreeNode> AssetNode = DependencyTree.Add(AssetId, MakeShared<FAssetTreeNode>(AssetRegistry, AssetData));

	TArray<FName> Dependencies;
	AssetRegistry.GetDependencies(AssetData.PackageName, Dependencies, UE::AssetRegistry::EDependencyCategory::Package, UE::AssetRegistry::EDependencyQuery::Hard);
	
	for (FName Dependency: Dependencies)
	{
		FAssetIdentifier DependencyId{Dependency};
		const FString DependencyStr = Dependency.ToString();
		
		if (FPackageName::IsScriptPackage(DependencyStr))
		{
			continue;
		}

		if (VisitedAssets.Contains(Dependency))
		{
			continue;
		}

		TSharedPtr<FAssetTreeNode> DependencyNode{};
		if (TSharedPtr<FAssetTreeNode>* DependencyNodePtr = DependencyTree.Find(DependencyId))
		{
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

void UAVCommandletAction_AuditAssets::AuditAsset(TSharedPtr<FAssetTreeNode> AssetNode, FAssetAuditResult& OutResult, int32 CurrentDepth) const
{
	check(AssetNode.IsValid());
	
	OutResult.Add(AssetNode);

	OutResult.DependencyDepth = FMath::Max(OutResult.DependencyDepth, CurrentDepth);
	OutResult.MaxDependencyBreadth = FMath::Max(OutResult.MaxDependencyBreadth, AssetNode->Children.Num());
	OutResult.TotalDependencyCount += AssetNode->Children.Num();
	
	for (const TSharedPtr<FAssetTreeNode>& Child: AssetNode->Children)
	{
		AuditAsset(Child, OutResult, CurrentDepth + 1);
	}
}
