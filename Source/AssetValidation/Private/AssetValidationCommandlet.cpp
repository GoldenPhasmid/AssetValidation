#include "AssetValidationCommandlet.h"

#include "AssetValidationSettings.h"
#include "AssetValidationStatics.h"
#include "EditorValidatorBase.h"
#include "EditorValidatorSubsystem.h"
#include "WidgetBlueprint.h"
#include "Algo/RemoveIf.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/UserDefinedEnum.h"
#include "Engine/UserDefinedStruct.h"
#include "Sound/SoundClass.h"

/**
 * Behavior commandline params
 */
namespace UE::AssetValidation
{
	static const TCHAR* Separator{TEXT(",")};
	/** Enable detailed logging during validation */
	static const FString DetailedLog{TEXT("DetailedLog")};
	/** Parameter, disable one or more editor validators */
	static const FString DisableValidators{TEXT("DisableValidators")};
	/** Do not perform any validation, just output the number of assets found */
	static const FString OnlyCount{TEXT("OnlyCount")};
}

/**
 * path commandline params
 */
namespace UE::AssetValidation
{
	/** Switch, adds all game content path to validating assets by adding /Game to 'PackagePaths'. Same as '-Paths=/Game' */
	static const FString GameContent{TEXT("GameContent")};
	
	/**
	 * Parameter, specify a list of package paths to validate.
	 * Expects a list of package paths starting with /, using ',' as a separator. Example: '-Paths=/Game,/MyGamePlugin'
	 */
	static const FString Paths{TEXT("Paths")};
}

/**
 * asset type commandline params
 */
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


int32 UAssetValidationCommandlet::Main(const FString& Commandline)
{
	UE_LOG(LogAssetValidation, Display, TEXT("Running UAssetValidationCommandlet"));
	
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	AssetRegistry.SearchAllAssets(true);
	
	TArray<FString> Tokens, Switches;
	TMap<FString, FString> Params;
	ParseCommandLine(*Commandline, Tokens, Switches, Params);
	
	TArray<FAssetData> AllAssets;

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bIncludeOnlyOnDiskAssets = true;
	Filter.PackagePaths = GetPackagePaths(Params, Switches);
	Filter.ClassPaths = GetAllowedClasses(Params, Switches);
	AssetRegistry.GetAssets(Filter, AllAssets);
	
	// a list of specific maps to validate
	TArray<FName> MapNames;
	if (const FString* Values = Params.Find(UE::AssetValidation::Maps))
	{
		TArray<FString> Temp;
		Values->ParseIntoArray(Temp, UE::AssetValidation::Separator);

		MapNames.Reserve(Temp.Num());
		Algo::Transform(Temp, MapNames, [](const FString& MapName) { return FName{MapName}; });
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
	AllAssets.SetNum(Algo::RemoveIf(AllAssets, IsAssetPackageExternalOrFilteredMap));

	if (Switches.Contains(UE::AssetValidation::OnlyCount))
	{
		UE_LOG(LogAssetValidation, Display, TEXT("UAssetValidationCommandlet: Found %d assets. Exiting"), AllAssets.Num());
		return 0;
	}
	
	FValidateAssetsSettings Settings;
	Settings.bSkipExcludedDirectories = true;
	Settings.bShowIfNoFailures = true;
	Settings.ValidationUsecase = EDataValidationUsecase::Commandlet;
	
	if (const FString* Values = Params.Find(UE::AssetValidation::DisableValidators))
	{
		TArray<FString> ClassNames;
		Values->ParseIntoArray(ClassNames, UE::AssetValidation::Separator);

		DisableValidators(ClassNames);
	}

	// enable or disable verbose asset logging depending on a switch
	UAssetValidationSettings::GetMutable()->bEnabledDetailedAssetLogging = Switches.Contains(UE::AssetValidation::DetailedLog);

	FValidateAssetsResults Result = ValidateAssets(AllAssets, Settings);
	return Result.NumInvalid;
}

void UAssetValidationCommandlet::DisableValidators(TConstArrayView<FString> ClassNames)
{
	TSet<UClass*> ValidatorClasses;
    for (const FString& ClassName: ClassNames)
    {
    	if (UClass* ValidatorClass = UClass::TryFindTypeSlow<UClass>(ClassName))
    	{
    		ValidatorClasses.Add(ValidatorClass);
    	}
    }

	UAssetValidationSubsystem* Subsystem = UAssetValidationSubsystem::Get();
	Subsystem->ForEachEnabledValidator([&ValidatorClasses](UEditorValidatorBase* EditorValidator)
	{
		if (ValidatorClasses.Contains(EditorValidator->GetClass()))
		{
			UE::AssetValidation::SetValidatorEnabled(EditorValidator, false);
		}
		return true;
	});
}

TArray<FName> UAssetValidationCommandlet::GetPackagePaths(const TMap<FString, FString>& Params, TConstArrayView<FString> Switches) const
{
	TArray<FName> PackagePaths;
	if (Switches.Contains(UE::AssetValidation::GameContent))
	{
		PackagePaths.Add(TEXT("/Game"));
	}
	
	if (const FString* Values = Params.Find(UE::AssetValidation::Paths))
	{
		TArray<FString> Paths;
		Values->ParseIntoArray(Paths, UE::AssetValidation::Separator);
		
		PackagePaths.Append(Paths);
	}
	
	return PackagePaths;
}

TArray<FTopLevelAssetPath> UAssetValidationCommandlet::GetAllowedClasses(const TMap<FString, FString>& Params, TConstArrayView<FString> Switches) const
{
	TArray<FTopLevelAssetPath> AllowedClasses;
	if (Switches.Contains(UE::AssetValidation::AllAssets))
	{
		return AllowedClasses;
	}

	// blueprint assets
	if (Switches.Contains(UE::AssetValidation::Blueprints))
	{
		AllowedClasses.Add(UBlueprint::StaticClass()->GetClassPathName());
		AllowedClasses.Add(UDataTable::StaticClass()->GetClassPathName());
		AllowedClasses.Add(UDataAsset::StaticClass()->GetClassPathName());
		AllowedClasses.Add(UUserDefinedStruct::StaticClass()->GetClassPathName());
		AllowedClasses.Add(UUserDefinedEnum::StaticClass()->GetClassPathName());
	}
	// widgets
	if (Switches.Contains(UE::AssetValidation::Widgets))
	{
		AllowedClasses.Add(UWidgetBlueprint::StaticClass()->GetClassPathName());
		AllowedClasses.Add(UUserWidgetBlueprint::StaticClass()->GetClassPathName());
		AllowedClasses.Add(UBaseWidgetBlueprint::StaticClass()->GetClassPathName());
	}
	// animations, montages and anim BPs
	if (Switches.Contains(UE::AssetValidation::Animations))
	{
		AllowedClasses.Add(UAnimationAsset::StaticClass()->GetClassPathName());
		AllowedClasses.Add(UAnimMontage::StaticClass()->GetClassPathName());
		AllowedClasses.Add(UAnimBlueprint::StaticClass()->GetClassPathName());
	}
	// unreal sounds
	if (Switches.Contains(UE::AssetValidation::Sounds))
	{
		AllowedClasses.Add(USoundWave::StaticClass()->GetClassPathName());
		AllowedClasses.Add(USoundClass::StaticClass()->GetClassPathName());
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
				AllowedClasses.Add(Class->GetClassPathName());
			}
		}
	}

	// add world asset type if we have -AllMaps or specified list of maps with -Maps. -Maps are getting filtered later
	if (Switches.Contains(UE::AssetValidation::AllMaps) || Params.Contains(UE::AssetValidation::Maps))
	{
		AllowedClasses.Add(UWorld::StaticClass()->GetClassPathName());
	}

	return AllowedClasses;
}

FValidateAssetsResults UAssetValidationCommandlet::ValidateAssets(const TArray<FAssetData>& Assets, FValidateAssetsSettings& Settings) const
{
	UAssetValidationSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetValidationSubsystem>();

	FValidateAssetsResults Results;
	Subsystem->ValidateAssetsWithSettings(Assets, Settings, Results);
	return Results;
}
