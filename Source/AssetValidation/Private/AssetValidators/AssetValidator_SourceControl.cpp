#include "AssetValidators/AssetValidator_SourceControl.h"

#include "FileHelpers.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "SourceControlHelpers.h"
#include "SourceControlOperations.h"
#include "AssetRegistry/AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

bool UAssetValidator_SourceControl::CanValidateAsset_Implementation(UObject* InAsset) const
{
	return Super::CanValidateAsset_Implementation(InAsset) && InAsset != nullptr;
}

EDataValidationResult UAssetValidator_SourceControl::ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors)
{
	check(InAsset);
	
	const FString PackageName = InAsset->GetPackage()->GetName();
	if (!FPackageName::DoesPackageExist(PackageName) && !PackageName.StartsWith("/Script/"))
	{
		AssetFails(InAsset, FText::Format(LOCTEXT("SourceControl_InvalidAsset", "Asset {0} is part of package {1} which doesn't exist"),
						FText::FromString(InAsset->GetName()), FText::FromString(PackageName)), ValidationErrors);
		return EDataValidationResult::Invalid;
	}

	ISourceControlProvider& SCCProvider = ISourceControlModule::Get().GetProvider();
	FSourceControlStatePtr AssetState = SCCProvider.GetState(SourceControlHelpers::PackageFilename(PackageName), EStateCacheUsage::Use);

	if (!AssetState.IsValid() || !AssetState->IsSourceControlled())
	{
		AssetPasses(InAsset);
		return EDataValidationResult::Valid;
	}
	
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	const IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// analyze dependencies 
	TArray<FName> Dependencies;
	AssetRegistry.GetDependencies(FName(PackageName), Dependencies, UE::AssetRegistry::EDependencyCategory::Package);
		
	for (FName DependencyName: Dependencies)
	{
		FString Dependency = DependencyName.ToString();
		if (!Dependency.StartsWith(TEXT("/Script/")))
		{
			FSourceControlStatePtr DependencyState = SCCProvider.GetState(SourceControlHelpers::PackageFilename(Dependency), EStateCacheUsage::Use);
			if (!DependencyState.IsValid())
			{
				continue;
			}
			if (!DependencyState->IsSourceControlled() && !DependencyState->IsUnknown())
			{
				// The editor doesn't sync state for all assets, so we only want to warn on assets that are known about
				AssetFails(InAsset, FText::Format(LOCTEXT("SourceControl_NotMarkedForAdd", "Asset {0} references {1} which is not marked for add to source control"),
								FText::FromString(PackageName), FText::FromString(Dependency)), ValidationErrors);
			}
			if (!DependencyState->IsCurrent())
			{
				AssetFails(InAsset, FText::Format(LOCTEXT("SourceControl_NotLatestRevision", "Asset {0} references {1} which is not on latest revision"),
					FText::FromString(PackageName), FText::FromString(Dependency)), ValidationErrors);
			}
		}
	}
	
	if (GetValidationResult() != EDataValidationResult::Invalid)
	{
		AssetPasses(InAsset);
	}

	return GetValidationResult();
}

#undef LOCTEXT_NAMESPACE