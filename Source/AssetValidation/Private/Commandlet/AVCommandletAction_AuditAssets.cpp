#include "Commandlet/AVCommandletAction_AuditAssets.h"

#include "AssetValidationDefines.h"
#include "AssetValidationStatics.h"
#include "AssetDependencyTree.h"
#include "Algo/IndexOf.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Misc/EnumerateRange.h"

namespace UE::AssetValidation
{
	static const FString DisplaySummary{TEXT("Summary")};
	static const FString MaxCount{TEXT("MaxCount")};
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

	TArray<FAssetAuditResult> Results;
	Results.Reserve(NumAssets);
	
	FAssetDependencyTree DependencyTree{};
	for (TEnumerateRef<const FAssetData> AssetData: EnumerateRange(Assets))
	{
		FAssetAuditResult& Result = Results.AddDefaulted_GetRef();
		
		DependencyTree.AuditAsset(AssetRegistry, *AssetData, Result);
		
		constexpr float MB = 1.0 / 1024.0 / 1024.0;
		UE_LOG(LogAssetValidation, Display, TEXT("audit asset %s: %d/%d. Memory size: %.2fMB, Disk size: %.2fMB"), *AssetData->AssetName.ToString(), AssetData.GetIndex(), NumAssets, Result.TotalMemorySizeBytes * MB, Result.TotalDiskSizeBytes * MB);
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
