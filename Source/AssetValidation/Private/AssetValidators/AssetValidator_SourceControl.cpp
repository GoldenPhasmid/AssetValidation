#include "AssetValidators/AssetValidator_SourceControl.h"

#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "SourceControlHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

bool UAssetValidator_SourceControl::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	return Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext) && InObject != nullptr;
}

EDataValidationResult UAssetValidator_SourceControl::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	check(InAsset);
	
	const FString PackageName = InAsset->GetPackage()->GetName();
	if (!FPackageName::DoesPackageExist(PackageName) && !PackageName.StartsWith("/Script/"))
	{
		const FText FailReason = FText::Format(LOCTEXT("SourceControl_InvalidAsset", "Asset {0} is part of package {1} which doesn't exist"),
                                   FText::FromString(InAsset->GetName()), FText::FromString(PackageName));
		AssetFails(InAsset, FailReason);
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
				// ignore engine assets as we're not allowed to change them anyway or they can be under different repo
				if (!bIgnoreEngineDependencies || !Dependency.Contains(TEXT("/Engine/")))
				{
					const FText FailReason =	FText::Format(LOCTEXT("SourceControl_NotMarkedForAdd", "Asset {0} references {1} which is not marked for add to source control"),
												FText::FromString(PackageName), FText::FromString(Dependency));
					// The editor doesn't sync state for all assets, so we only want to warn on assets that are known about
					AssetFails(InAsset, FailReason);
				}

			}
			if (!DependencyState->IsCurrent()) // @todo: this check is not implemented for git :)
			{
				const FText FailReason =	FText::Format(LOCTEXT("SourceControl_NotLatestRevision", "Asset {0} references {1} which is not on latest revision"),
											FText::FromString(PackageName), FText::FromString(Dependency));
				AssetFails(InAsset, FailReason);
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