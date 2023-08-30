#include "ContentValidationCommandlet.h"

#include "EditorValidatorSubsystem.h"
#include "Algo/RemoveIf.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"

int32 UContentValidationCommandlet::Main(const FString& Commandline)
{
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	AssetRegistry.SearchAllAssets(true);
	
	TArray<FString> Tokens, Switches;
	TMap<FString, FString> Params;
	ParseCommandLine(*Commandline, Tokens, Switches, Params);
	
	TArray<FAssetData> AssetList;

	FARFilter Filter;
	Filter.bRecursivePaths = true;
	
	if (Switches.Contains(TEXT("Game")))
	{
		Filter.PackagePaths.Add("/Game");
	}
	else if (FString* Values = Params.Find(TEXT("Paths")))
	{
		TArray<FString> Paths;
		Values->ParseIntoArray(Paths, TEXT(","));
		
		Filter.PackagePaths.Append(Paths);
	}
	AssetRegistry.GetAssets(Filter, AssetList);

	// Remove ExternalActors & ExternalObjects from assets to be validated.
	// If ExternalActors are not loaded, they will spam the validation log as they can't
	// be loaded on the fly like other assets.
	auto IsAssetPackageExternal = [](const FAssetData& AssetData)
	{
		FString ObjectPath = AssetData.GetObjectPathString();
		FStringView ClassName, PackageName, ObjectName, SubObjectName;
		FPackageName::SplitFullObjectPath(FStringView(ObjectPath), ClassName, PackageName, ObjectName, SubObjectName);

		return FName(PackageName) != AssetData.PackageName;
	};
	AssetList.SetNum(Algo::RemoveIf(AssetList, IsAssetPackageExternal));
	
	FValidateAssetsSettings Settings;
	Settings.bSkipExcludedDirectories = true;
	Settings.bShowIfNoFailures = true;
	Settings.ValidationUsecase = EDataValidationUsecase::Commandlet;
	FValidateAssetsResults Results;
	
	UEditorValidatorSubsystem* ValidatorSubsystem = GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>();
	ValidatorSubsystem->ValidateAssetsWithSettings(AssetList, Settings, Results);

	return Results.NumInvalid > 0 ? 1 : 0;
}
