#include "Commandlet/AVCommandletAssetSearchFilter.h"

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
	 * Switch, will not use asset filter when querying asset validation.
	 * Exclusive with all other asset type options.
	 */
	static const FString AllAssets{TEXT("All")};
	/** Adds BP assets and data only assets to validation */
	static const FString Blueprints{TEXT("Blueprints")};
	/** Adds widget assets to validation */
	static const FString Widgets{TEXT("Widgets")};
	/** Adds animations, montages and anim BPs to validation */
	static const FString Animations{TEXT("Animations")};
	/** Adds unreal sound assets to validation */
	static const FString Sounds{TEXT("Sounds")};
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
	bBlueprints = Switches.Contains(UE::AssetValidation::Blueprints);
	bWidgets = Switches.Contains(UE::AssetValidation::Widgets);
	bAnimations = Switches.Contains(UE::AssetValidation::Animations);
	bSounds = Switches.Contains(UE::AssetValidation::Sounds);
	
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
	TArray<FTopLevelAssetPath> AllowedClasses;
	if (bAllAssetTypes)
	{
		return AllowedClasses;
	}

	AllowedClasses.Reserve(AssetTypes.Num() + 20);
	// blueprint assets
	if (bBlueprints)
	{
		AllowedClasses.Add(UBlueprint::StaticClass()->GetClassPathName());
		AllowedClasses.Add(UDataTable::StaticClass()->GetClassPathName());
		AllowedClasses.Add(UDataAsset::StaticClass()->GetClassPathName());
		AllowedClasses.Add(UUserDefinedStruct::StaticClass()->GetClassPathName());
		AllowedClasses.Add(UUserDefinedEnum::StaticClass()->GetClassPathName());
	}
	// widgets
	if (bWidgets)
	{
		AllowedClasses.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());
		AllowedClasses.Add(UUserWidgetBlueprint::StaticClass()->GetClassPathName());
		AllowedClasses.Add(UBaseWidgetBlueprint::StaticClass()->GetClassPathName());
	}
	// animations, montages and anim BPs
	if (bAnimations)
	{
		AllowedClasses.Add(UAnimationAsset::StaticClass()->GetClassPathName());
		AllowedClasses.Add(UAnimBlueprint::StaticClass()->GetClassPathName());

		TArray<UClass*> AnimationClasses;
		GetDerivedClasses(UAnimationAsset::StaticClass(), AnimationClasses);

		for (const UClass* Class: AnimationClasses)
		{
			AllowedClasses.Add(Class->GetClassPathName());
		}
	}
	
	// unreal sounds
	if (bSounds)
	{
		TArray<UClass*> SoundClasses;
		GetDerivedClasses(USoundBase::StaticClass(), SoundClasses);
		
		for (const UClass* Class: SoundClasses)
		{
			AllowedClasses.Add(Class->GetClassPathName());
		}
	}

	// add world asset type if we have -AllMaps or specified list of maps with -Maps. -Maps are getting filtered later
	if (bAllMaps || !Maps.IsEmpty())
	{
		AllowedClasses.Add(UWorld::StaticClass()->GetClassPathName());
	}

	// additional asset types
	for (const UClass* AssetType: AssetTypes)
	{
		check(AssetType);
		AllowedClasses.Add(AssetType->GetClassPathName());
	}
	
	return AllowedClasses;
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
