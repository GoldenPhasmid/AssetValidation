#include "Commandlet/AVCommandletAction_AuditAssets.h"

#include "AssetValidationDefines.h"
#include "AssetDependencyTree.h"
#include "AssetValidationSettings.h"
#include "Algo/IndexOf.h"
#include "Algo/RandomShuffle.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Commandlet/AVCommandletUtilities.h"
#include "Misc/EnumerateRange.h"
#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

namespace UE::AssetValidation
{
	static const FString DisplaySummary{TEXT("Summary")};
}

constexpr float MB = 1.0 / 1024.0 / 1024.0;

FAVCommandletActionResultBase::FAVCommandletActionResultBase(const FAssetData& AssetData)
	: AssetName(AssetData.AssetName.ToString())
	, AssetPath(AssetData.GetFullName())
{
	
}

FAVCommandletAction_AuditAssetResult::FAVCommandletAction_AuditAssetResult(const FAssetAuditResult& InResult)
	: Super(InResult.AssetData)
	, DiskSizeMB(InResult.TotalDiskSizeBytes * MB)
	, MemorySizeMB(InResult.TotalMemorySizeBytes * MB)
	, DependencyCount(InResult.TotalDependencyCount)
	, DependencyChainDepth(InResult.DependencyDepth)
{

}

void UAVCommandletAction_AuditAssets::InitFromCommandlet(const TArray<FString>& Switches, const TMap<FString, FString>& Params)
{
	bDisplaySummary = Switches.Contains(UE::AssetValidation::DisplaySummary);
}

bool UAVCommandletAction_AuditAssets::Run(const TArray<FAssetData>& InAssets)
{
	if (InAssets.IsEmpty())
	{
		return true;
	}
	
 	IAssetRegistry& AssetRegistry = IAssetRegistry::GetChecked();
	const int32 NumAssets = InAssets.Num();
	
	TArray<FAVCommandletAction_AuditAssetResult> Results;
	Results.Reserve(NumAssets);

	FScopedSlowTask SlowTask(NumAssets, LOCTEXT("UAVCommandletAction_AuditAssetsTask", "Audit Assets..."));
	SlowTask.MakeDialog(NumAssets > UAssetValidationSettings::Get()->NumAssetsToShowCancelButton);
	
	for (TEnumerateRef<const FAssetData> AssetData: EnumerateRange(InAssets))
	{
		if (SlowTask.ShouldCancel())
		{
			break;	
		}
		SlowTask.EnterProgressFrame(1.0f);
		
		FAssetAuditResult AuditResult{*AssetData};
		FAssetDependencyTree{}.AuditAsset(AssetRegistry, *AssetData, AuditResult);

		Results.Add(FAVCommandletAction_AuditAssetResult{AuditResult});
	}
	
	const FString ExportText = UE::AssetValidation::CsvExport(Results);
	FFileHelper::SaveStringToFile(ExportText, *FPaths::ConvertRelativePathToFull(OutFile.FilePath));
	
	if (bDisplaySummary)
	{
		Algo::Sort(Results, [](const FAVCommandletAction_AuditAssetResult& Lhs, const FAVCommandletAction_AuditAssetResult& Rhs)
		{
			return	Lhs.MemorySizeMB < Rhs.MemorySizeMB
					|| FMath::IsNearlyEqual(Lhs.MemorySizeMB, Rhs.MemorySizeMB) && Lhs.DiskSizeMB < Rhs.DiskSizeMB
					|| FMath::IsNearlyEqual(Lhs.MemorySizeMB, Rhs.MemorySizeMB) && FMath::IsNearlyEqual(Lhs.DiskSizeMB, Rhs.DiskSizeMB) && Lhs.DependencyCount < Rhs.DependencyCount;
		});
		
		const int32 GigaMemorySizeNumAssets = Algo::IndexOfByPredicate(Results, [](const FAVCommandletAction_AuditAssetResult& Result)
		{
			return (Result.MemorySizeMB / 1024) > 1.0;
		});
		const int32 GigaDiskSizeNumAssets = Algo::IndexOfByPredicate(Results, [](const FAVCommandletAction_AuditAssetResult& Result)
		{
			return (Result.DiskSizeMB / 1024) > 1.0;
		});

		const FAVCommandletAction_AuditAssetResult& Result = Results[Results.Num() / 2];

		UE_LOG(LogAssetValidation, Display, TEXT("AuditAssets - Processed %d assets."), NumAssets);
		UE_LOG(LogAssetValidation, Display, TEXT("AuditAssets - Median Dependency Count: %d"), Result.DependencyCount);
		UE_LOG(LogAssetValidation, Display, TEXT("AuditAssets - Median Memory Size: %.2fKB"), Result.MemorySizeMB * 1024);
		UE_LOG(LogAssetValidation, Display, TEXT("AuditAssets - Median Disk Size: %.2fKB"), Result.DiskSizeMB * 1024);
		UE_LOG(LogAssetValidation, Display, TEXT("AuditAssets - Median Depth: %d"), Result.DependencyChainDepth);
		UE_LOG(LogAssetValidation, Display, TEXT("AuditAssets - Num assets > 1 GB in memory size: %d"), NumAssets - GigaMemorySizeNumAssets + 1);
		UE_LOG(LogAssetValidation, Display, TEXT("AuditAssets - Num assets > 1 GB in disk size: %d"), NumAssets - GigaDiskSizeNumAssets + 1);
	}

	return true;
}

#undef LOCTEXT_NAMESPACE
