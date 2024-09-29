#include "Commandlet/AVCommandletAssetSearchFilter.h"

#include "EditorUtilityWidgetBlueprint.h"
#include "WidgetBlueprint.h"
#include "Algo/RemoveIf.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/UserDefinedEnum.h"
#include "Engine/UserDefinedStruct.h"

/** PATHS */
namespace UE::AssetValidation
{
	static const TCHAR* Separator{TEXT(",")};
	
	/** Switch, adds all game content path to validating assets by adding /Game to 'PackagePaths'. Same as '-Paths=/Game' */
	static const FString GameContent{TEXT("GameContent")};
	
	/**
	 * Parameter, specify a list of package paths to validate.
	 * Expects a list of package paths starting with /, using ',' as a separator. Example: '-Paths=/Game,/MyGamePlugin'
	 */
	static const FString Paths{TEXT("Paths")};
}

/** ASSET TYPES */
namespace UE::AssetValidation
{
	/**
	 * Parameter, specify a list of asset types to validate.
	 * Expects a list of class names using ',' as a separator.
	 */
	static const FString AssetTypes{TEXT("AssetTypes")};
	/**
	 * Parameter
	 * Specify a list of maps to validate, exclusive with -AllMaps switch.
	 * Expects a list of map short names using ',' as a separator.
	 */
	static const FString Maps{TEXT("Maps")};
	/**
	 * Switch, will add all maps by specified package paths to validation.
	 * Exclusive with -Maps param
	 */
	static const FString AllMaps{TEXT("AllMaps")};
	
	/**
	 * Switch, will not use class path filter when querying assets.
	 * Exclusive with all other asset type options.
	 */
	static const FString AllAssets{TEXT("All")};
	/** Adds BP assets and data only assets to class paths */
	static const FString Blueprints{TEXT("Blueprints")};
	/** Adds widget assets to class paths */
	static const FString Widgets{TEXT("Widgets")};
	/** Adds editor utility widget assets to class paths */
	static const FString EditorUtilityWidgets{TEXT("EditorUtilityWidgets")};
	/** Adds animations, montages and anim BPs to class paths */
	static const FString Animations{TEXT("Animations")};
	/** Adds static meshes to class paths */
	static const FString StaticMeshes{TEXT("StaticMeshes")};
	/** Adds skeletal meshes to class paths */
	static const FString SkeletalMeshes{TEXT("SkeletalMeshes")};
	/** Adds material classes to class paths */
	static const FString Materials{TEXT("Materials")};
	/** Adds texture classes to class paths */
	static const FString Textures{TEXT("Textures")};
	/** Adds unreal sound assets to class paths */
	static const FString Sounds{TEXT("Sounds")};
	/** Adds unreal redirector type to class paths */
	static const FString Redirectors{TEXT("Redirectors")};
}


void UAVCommandletAssetSearchFilter::InitFromCommandlet(const TArray<FString>& Switches, const TMap<FString, FString>& Params)
{
	Super::InitFromCommandlet(Switches, Params);
	
	if (Switches.Contains(UE::AssetValidation::GameContent))
	{
		CommandletPackagePaths.Add(TEXT("/Game"));
	}

	if (const FString* Values = Params.Find(UE::AssetValidation::Paths))
	{
		TArray<FString> Paths;
		Values->ParseIntoArray(Paths, UE::AssetValidation::Separator);
		
		CommandletPackagePaths.Append(Paths);
	}

	// additional asset types
	if (const FString* Values = Params.Find(UE::AssetValidation::AssetTypes))
	{
		TArray<FString> ClassNames;
		Values->ParseIntoArray(ClassNames, UE::AssetValidation::Separator);

		for (const FString& ClassName: ClassNames)
		{
			if (const UClass* Class = UClass::TryFindTypeSlow<UClass>(ClassName))
			{
				AssetTypes.Add(Class);
			}
		}
	}

	bAllMaps = Switches.Contains(UE::AssetValidation::AllMaps);
	bAllAssetTypes = Switches.Contains(UE::AssetValidation::AllAssets);
	
	if (bAllMaps == false)
	{
		// map names
		if (const FString* Values = Params.Find(UE::AssetValidation::Maps))
		{
			TArray<FString> Temp;
			Values->ParseIntoArray(Temp, UE::AssetValidation::Separator);

			CommandletMapNames.Reserve(Temp.Num());
			Algo::Transform(Temp, CommandletMapNames, [](const FString& MapName) { return FName{MapName}; });
		}
	}
}

bool UAVCommandletAssetSearchFilter::GetAssets(TArray<FAssetData>& OutAssets) const
{
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	AssetRegistry.SearchAllAssets(true);

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bIncludeOnlyOnDiskAssets = true;
	Filter.PackagePaths = GetPackagePaths();
	AddPackagePaths(Filter.PackagePaths);
	
	Filter.ClassPaths = GetAllowedClasses();
	AddClassPaths(Filter.ClassPaths);
	
	AssetRegistry.GetAssets(Filter, OutAssets);
	
	// a list of specific maps to validate
	TArray<FName> MapNames;
	if (bAllMaps == false && !Maps.IsEmpty())
	{
		MapNames.Reserve(Maps.Num() + CommandletMapNames.Num());
		for (const TSoftObjectPtr<UWorld>& Map: Maps)
		{
			check(!Map.IsNull());
			MapNames.Add(FName{Map.GetAssetName()});
		}
		MapNames.Append(CommandletMapNames);
	}
	
	// Remove ExternalActors & ExternalObjects from assets to be validated.
	// Remove maps that were not specified by -Maps param, or it is not specified
	// If ExternalActors are not loaded, they will spam the validation log as they can't
	// be loaded on the fly like other assets.
	// Also, external actors are validated as a part of world asset validation
	auto IsAssetPackageExternalOrFilteredMap = [&MapNames](const FAssetData& AssetData)
	{
		FString ObjectPath = AssetData.GetObjectPathString();
		FStringView ClassName, PackageName, ObjectName, SubObjectName;
		FPackageName::SplitFullObjectPath(FStringView(ObjectPath), ClassName, PackageName, ObjectName, SubObjectName);

		if (FName{PackageName} != AssetData.PackageName)
		{
			return true;
		}

		static const FTopLevelAssetPath WorldClassPath = UWorld::StaticClass()->GetClassPathName();
		if (AssetData.AssetClassPath == WorldClassPath && !MapNames.IsEmpty() && !MapNames.Contains(AssetData.AssetName))
		{
			return true;			
		}

		return false;
	};
	OutAssets.SetNum(Algo::RemoveIf(OutAssets, IsAssetPackageExternalOrFilteredMap));

	FilterAssets(OutAssets);

	return true;
}

TArray<FTopLevelAssetPath> UAVCommandletAssetSearchFilter::GetAllowedClasses() const
{
	TArray<FTopLevelAssetPath> AllowedClassPaths;
	if (bAllAssetTypes)
	{
		return AllowedClassPaths;
	}

	auto AddDerivedClasses = [&AllowedClassPaths](const UClass* BaseClass)
	{
		TArray<UClass*> Classes;
		GetDerivedClasses(BaseClass, Classes);

		for (const UClass* DerivedClass: Classes)
		{
			AllowedClassPaths.Add(DerivedClass->GetClassPathName());
		}
	};

	AllowedClassPaths.Reserve(AssetTypes.Num() + 20);
	// blueprint assets
	if (bBlueprints)
	{
		AllowedClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
		AllowedClassPaths.Add(UDataTable::StaticClass()->GetClassPathName());
		AllowedClassPaths.Add(UDataAsset::StaticClass()->GetClassPathName());
		AllowedClassPaths.Add(UUserDefinedStruct::StaticClass()->GetClassPathName());
		AllowedClassPaths.Add(UUserDefinedEnum::StaticClass()->GetClassPathName());
	}
	// widgets
	if (bWidgets)
	{
		AllowedClassPaths.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());
		AllowedClassPaths.Add(UUserWidgetBlueprint::StaticClass()->GetClassPathName());
		AllowedClassPaths.Add(UBaseWidgetBlueprint::StaticClass()->GetClassPathName());
	}
	// editor utility widgets
	if (bEditorUtilityWidgets)
	{
		AllowedClassPaths.Add(UEditorUtilityWidgetBlueprint::StaticClass()->GetClassPathName());
	}
	// animations, montages and anim BPs
	if (bAnimations)
	{
		AllowedClassPaths.Add(UAnimationAsset::StaticClass()->GetClassPathName());
		AllowedClassPaths.Add(UAnimBlueprint::StaticClass()->GetClassPathName());

		AddDerivedClasses(UAnimationAsset::StaticClass());
	}
	// static meshes
	if (bStaticMeshes)
	{
		AllowedClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());
	}
	// skeletal meshes
	if (bSkeletalMeshes)
	{
		AllowedClassPaths.Add(USkeletalMesh::StaticClass()->GetClassPathName());
	}
	// materials
	if (bMaterials)
	{
		AddDerivedClasses(UMaterialInterface::StaticClass());
	}
	// textures
	if (bTextures)
	{
		AddDerivedClasses(UTexture::StaticClass());
	}
	// unreal sounds
	if (bSounds)
	{
		AddDerivedClasses(USoundBase::StaticClass());
	}
	// redirectors
	if (bRedirectors)
	{
		AllowedClassPaths.Add(UObjectRedirector::StaticClass()->GetClassPathName());
	}

	// add world asset type if we have -AllMaps or specified list of maps with -Maps. -Maps are getting filtered later
	if (bAllMaps || !Maps.IsEmpty())
	{
		AllowedClassPaths.Add(UWorld::StaticClass()->GetClassPathName());
	}

	// additional asset types
	for (const UClass* AssetType: AssetTypes)
	{
		check(AssetType);
		AllowedClassPaths.Add(AssetType->GetClassPathName());
	}
	
	return AllowedClassPaths;
}

TArray<FName> UAVCommandletAssetSearchFilter::GetPackagePaths() const
{
	TArray<FName> PackagePaths;
	PackagePaths.Reserve(DirectoryPaths.Num() + CommandletPackagePaths.Num());
	
	for (const FDirectoryPath& Path: DirectoryPaths)
	{
		PackagePaths.Add(FName{Path.Path});
	}

	PackagePaths.Append(CommandletPackagePaths);
	return PackagePaths;
}
