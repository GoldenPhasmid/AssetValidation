#pragma once

#include "CoreMinimal.h"
#include "AVCommandletAction.h"

#include "AVCommandletAction_AuditAssets.generated.h"

struct FAssetData;
class IAssetRegistry;
class IAssetManagerEditorModule;

struct FAssetTreeNode
{
	FAssetTreeNode() = default;
	FAssetTreeNode(IAssetRegistry& AssetRegistry, const FAssetData& InAssetData);

	bool IsValid() const;

	FAssetData AssetData;
	float TotalDiskSizeBytes = 0.f;
	float TotalMemorySizeBytes = 0.f;
	
	TArray<TSharedPtr<FAssetTreeNode>> Children;
};

using FAssetDependencyTree = TMap<FAssetIdentifier, TSharedPtr<FAssetTreeNode>>;

struct FAssetAuditResult
{
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

	FORCEINLINE FAssetAuditResult& operator+=(const FAssetAuditResult& Other)
	{
		TotalDiskSizeBytes += Other.TotalDiskSizeBytes;
		TotalMemorySizeBytes += Other.TotalMemorySizeBytes;
		TotalDependencyCount += Other.TotalDependencyCount;
		MaxDependencyBreadth += Other.MaxDependencyBreadth;
		DependencyDepth += Other.DependencyDepth;

		return *this;
	}
};

FORCEINLINE FAssetAuditResult operator+(const FAssetAuditResult& Lhs, const FAssetAuditResult& Rhs)
{
	FAssetAuditResult Result = Lhs;
	Result += Rhs;
	return Result;
}

UCLASS(DisplayName = "Audit Assets")
class ASSETVALIDATION_API UAVCommandletAction_AuditAssets: public UAVCommandletAction
{
	GENERATED_BODY()
public:

	virtual void InitFromCommandlet(const TArray<FString>& Switches, const TMap<FString, FString>& Params) override;
	virtual bool Run(const TArray<FAssetData>& Assets) override;
	
	void AuditAsset(TSharedPtr<FAssetTreeNode> AssetNode, FAssetAuditResult& OutResult, int32 CurrentDepth = 1) const;
	TSharedPtr<FAssetTreeNode> CreateAssetNode(IAssetRegistry& AssetRegistry, const FAssetData& AssetData, FAssetDependencyTree& DependencyTree, TSet<FName>& VisitedAssets) const;
	
	UPROPERTY(EditAnywhere)
	FString OutputFile;

	UPROPERTY(EditAnywhere)
	int32 MaxCount = 0;
	
	UPROPERTY(EditAnywhere)
	bool bDisplaySummary = false;
};
