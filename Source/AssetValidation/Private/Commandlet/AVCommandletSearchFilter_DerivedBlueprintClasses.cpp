#include "Commandlet/AVCommandletSearchFilter_DerivedBlueprintClasses.h"

#include "AssetValidationSettings.h"
#include "Algo/RemoveIf.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"

namespace UE::AssetValidation
{
	static const FString BaseClass{TEXT("BaseClass")};
}

UAVCommandletSearchFilter_DerivedBlueprintClasses::UAVCommandletSearchFilter_DerivedBlueprintClasses()
{
}

void UAVCommandletSearchFilter_DerivedBlueprintClasses::PostInitProperties()
{
	Super::PostInitProperties();
	
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		Paths = UAssetValidationSettings::Get()->ValidatePaths;
	}
}

void UAVCommandletSearchFilter_DerivedBlueprintClasses::InitFromCommandlet(const TArray<FString>& Switches, const TMap<FString, FString>& Params)
{
	Super::InitFromCommandlet(Switches, Params);
}

bool UAVCommandletSearchFilter_DerivedBlueprintClasses::GetAssets(TArray<FAssetData>& OutAssets) const
{
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	AssetRegistry.SearchAllAssets(true);
	
	TArray<UClass*> Classes;
	GetDerivedClasses(BaseClass, Classes);
	Classes.Add(BaseClass);
	
	TArray<FTopLevelAssetPath> BaseClasses;
	BaseClasses.Reserve(Classes.Num());

	for (UClass* Class: Classes)
	{
		BaseClasses.Add(Class->GetClassPathName());
	}
	
	TSet<FTopLevelAssetPath> DerivedClasses;
	AssetRegistry.GetDerivedClassNames(BaseClasses, {}, DerivedClasses);
	
	for (auto It = DerivedClasses.CreateIterator(); It; ++It)
	{
		const FName PackageName = It->GetPackageName();
		const FString& PackageNameStr = PackageName.ToString();
		if (FPackageName::IsScriptPackage(PackageNameStr))
		{
			continue;
		}

		bool bPathFilterPassed = Paths.IsEmpty();
		for (const FDirectoryPath& Path: Paths)
		{
			if (PackageNameStr.StartsWith(Path.Path))
			{
				bPathFilterPassed = true;
				break;
			}
		}

		if (!bPathFilterPassed)
		{
			continue;
		}

		AssetRegistry.GetAssetsByPackageName(PackageName, OutAssets, false);
	}

	// leave only unique assets
	OutAssets = TSet<FAssetData>{OutAssets}.Array();

	return true;
}
