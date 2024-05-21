#include "AssetValidators/AssetValidator_Referencers.h"

#include "AssetValidationStatics.h"
#include "Algo/RemoveIf.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetValidators/AssetValidator_LoadPackage.h"
#include "Kismet2/BlueprintEditorUtils.h"

UAssetValidator_Referencers::UAssetValidator_Referencers()
{
	// validation on save would be too slow for any asset change
	AllowedContext &= ~EAssetValidationFlags::Save;
	// during commandlet validation we would most likely will validate all assets
	AllowedContext &= ~EAssetValidationFlags::Commandlet;
}

bool UAssetValidator_Referencers::IsEnabled() const
{
	return !IsRunningCommandlet() && Super::IsEnabled();
}

bool UAssetValidator_Referencers::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	if (!Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext))
	{
		return false;
	}
	
	// @todo: validate referencers for external actors? world partition already handles it?
	return InObject && (InObject->IsA<UBlueprint>() || InObject->IsA<UMaterialFunction>());
}

EDataValidationResult UAssetValidator_Referencers::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_Referencers, AssetValidationChannel);
	
	check(InAsset);
	if (UBlueprint* Blueprint = Cast<UBlueprint>(InAsset);
		Blueprint && FBlueprintEditorUtils::IsDataOnlyBlueprint(Blueprint))
	{
		// don't load referencers for data only blueprints, as they cannot break other blueprints from internal changes
		return EDataValidationResult::Valid;
	}

	UClass* AssetClass = InAsset->GetClass();
	IAssetRegistry& AssetRegistry = IAssetRegistry::GetChecked();

	TSet<FName> AllReferencers;
	TSet<FName> ProcessedPackages;
	TArray<FName, TInlineAllocator<16>> Packages{InAsset->GetPackage()->GetFName()};
	while (Packages.Num())
	{
		TArray<FName, TInlineAllocator<16>> NextPackages;
		for (const FName& Package: Packages)
		{
			if (ProcessedPackages.Contains(Package))
			{
				continue;
			}
			
			// find closest referencers
			FARFilter Filter;
			Filter.bIncludeOnlyOnDiskAssets = true;
			AssetRegistry.GetReferencers(Package, Filter.PackageNames, UE::AssetRegistry::EDependencyCategory::Package, UE::AssetRegistry::EDependencyQuery::Hard);

			// append processed packages before any filtering so that we don't get stuck in an infinite loop
			ProcessedPackages.Append(Filter.PackageNames);
			// filter referencers that we don't want to validate
			Filter.PackageNames.SetNum(Algo::RemoveIf(Filter.PackageNames, [](FName PackageName) { return !UE::AssetValidation::ShouldValidatePackage(PackageName.ToString()); }));
			
			TArray<FAssetData> PackageAssets;
			AssetRegistry.GetAssets(Filter, PackageAssets);

			// add redirectors as a next bunch to process
			for (const FAssetData& Asset: PackageAssets)
			{
				if (Asset.PackageFlags & (PKG_ContainsMap | PKG_ContainsMapData))
				{
					// ignore map related assets
					continue;
				}
				
				if (Asset.IsRedirector())
				{
					// encountered redirector, search farther
					NextPackages.AddUnique(Asset.PackageName);
				}
				
				FSoftClassPath ClassPath{Asset.AssetClassPath.ToString()};
				if (ClassPath.TryLoadClass<UObject>()->IsChildOf(AssetClass))
				{
					// add package to referencers only if it contains asset with a similar class type as validating dependency
					// this makes blueprints check other blueprints, material functions check material functions and so on
					AllReferencers.Add(Asset.PackageName);
				}
			}
		}

		Packages = MoveTemp(NextPackages);
	}

	TArray<FString> Warnings, Errors;
	for (const FName& PackageName: AllReferencers)
	{
		// load referencer packages and gather errors
		UAssetValidator_LoadPackage::GetPackageLoadErrors(PackageName.ToString(), FAssetData{}, Context);
	}

	if (Errors.Num() == 0)
	{
		AssetPasses(InAsset);
	}

	for (const FString& Warning: Warnings)
	{
		AssetWarning(InAsset, FText::FromString(Warning));
	}
	for (const FString& Error: Errors)
	{
		AssetFails(InAsset, FText::FromString(Error));
	}
	
	return GetValidationResult();
}
