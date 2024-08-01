#include "AssetValidators/AssetValidator_SourceControl.h"

#include "AssetValidationDefines.h"
#include "AssetValidationStatics.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "SourceControlHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

bool UAssetValidator_SourceControl::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	return Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext) && InAssetData.IsTopLevelAsset() && InObject != nullptr;
}

EDataValidationResult UAssetValidator_SourceControl::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_SourceControl, AssetValidationChannel);
	check(InAsset);
	
	const FString PackageName = InAsset->GetPackage()->GetName();
	// cache filename to use for source control queries and avoid SourceControlHelpers::PackageFilename
	FString PackageFilename{}; 
	if (!FPackageName::DoesPackageExist(PackageName, &PackageFilename)
		&& !FPackageName::IsScriptPackage(PackageName)
		&& !PackageName.StartsWith(TEXT("/Engine/Transient"))
		&& !PackageName.StartsWith(TEXT("/Temp"))) // game temp packages @todo investigate
	{
		const FText FailReason = FText::Format(LOCTEXT("SourceControl_InvalidAsset", " is part of package {0} which doesn't exist"), FText::FromString(PackageName));
		
		Context.AddMessage(InAsset, EMessageSeverity::Error, FailReason);
		return EDataValidationResult::Invalid;
	}

	ISourceControlProvider& SCCProvider = ISourceControlModule::Get().GetProvider();
	FSourceControlStatePtr AssetState = SCCProvider.GetState(PackageFilename, EStateCacheUsage::Use);
	
	EDataValidationResult Result = EDataValidationResult::Valid;
	if (!AssetState.IsValid() || !AssetState->IsSourceControlled())
	{
		AssetPasses(InAsset);
		return Result;
	}
	
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	const IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// analyze dependencies 
	TArray<FName> Dependencies;
	AssetRegistry.GetDependencies(FName(PackageName), Dependencies, UE::AssetRegistry::EDependencyCategory::Package);

	const bool bWorldAsset = UE::AssetValidation::IsWorldAsset(InAssetData);
	for (FName DependencyName: Dependencies)
	{
		FString Dependency = DependencyName.ToString();
		if (!FPackageName::IsScriptPackage(Dependency))
		{
			if (bWorldAsset && UE::AssetValidation::IsExternalAsset(Dependency))
			{
				continue;
			}
			
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
					const FText FailReason = FText::Format(LOCTEXT("SourceControl_NotMarkedForAdd", "references {0} which is not marked for add to source control"), FText::FromString(Dependency));
					// The editor doesn't sync state for all assets, so we only want to warn on assets that are known about
					Context.AddMessage(InAsset, EMessageSeverity::Error, FailReason);
					Result &= EDataValidationResult::Invalid;
				}
			}
			if (!DependencyState->IsCurrent()) // @todo: this check is not implemented for git :)
			{
				const FString FullName = InAssetData.AssetName.ToString();
				const FString AssetName = InAssetData.IsTopLevelAsset() ? PackageName : FullName;
				const FText FailReason = FText::Format(LOCTEXT("SourceControl_NotLatestRevision", "references {0} which is not on latest revision"), FText::FromString(Dependency));
				
				Context.AddMessage(InAsset, EMessageSeverity::Error, FailReason);
				Result &= EDataValidationResult::Invalid;
			}
		}
	}
	
	if (Result != EDataValidationResult::Invalid)
	{
		AssetPasses(InAsset);
	}

	return Result;
}

#undef LOCTEXT_NAMESPACE