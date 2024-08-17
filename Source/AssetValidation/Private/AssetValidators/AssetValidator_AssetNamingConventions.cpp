#include "AssetValidators/AssetValidator_AssetNamingConventions.h"

#include "AssetValidationDefines.h"
#include "AssetValidationStatics.h"
#include "Misc/DataValidation.h"
#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

UAssetValidator_AssetNamingConventions::UAssetValidator_AssetNamingConventions()
{
	bIsConfigDisabled = false; // enabled by default
}

bool UAssetValidator_AssetNamingConventions::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	if (!Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext))
	{
		return false;
	}
	
	const FString PackageName = InAssetData.PackageName.ToString();
	// validate on blueprint top level assets (basically assets from content browser)
	return UE::AssetValidation::IsBlueprintGeneratedPackage(PackageName) && InAssetData.IsTopLevelAsset();
}

EDataValidationResult UAssetValidator_AssetNamingConventions::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext)
{
	return ValidateAsset_Implementation(InAssetData, InContext);
}

EDataValidationResult UAssetValidator_AssetNamingConventions::ValidateAsset_Implementation(const FAssetData& InAssetData, FDataValidationContext& InContext)
{
	static const FRegexPattern AllowedCharacters{TEXT("^[A-Za-z0-9_-]*$")};

	EDataValidationResult Result = EDataValidationResult::Valid;

	const FString PackageName = FPackageName::GetShortName(InAssetData.PackageName);
	if (PackageName.IsEmpty())
	{
		InContext.AddMessage(UE::AssetValidation::CreateTokenMessage(
			EMessageSeverity::Error, InAssetData,
			LOCTEXT("AssetNamingConvention_InvalidPackage", "Failed to find a valid package name."))
		);
		Result &= EDataValidationResult::Invalid;
	}
	// fast check for first letter
	else if (PackageName[0] < TCHAR{'A'} || PackageName[0] > TCHAR{'Z'})
	{
		InContext.AddMessage(UE::AssetValidation::CreateTokenMessage(
			EMessageSeverity::Error, InAssetData,
			LOCTEXT("AssetNamingConvention_InitialCharacter", "Initial character should be a capitalized A-Z letter."))
		);
		Result &= EDataValidationResult::Invalid;
	}
	else if (FRegexMatcher Matcher{AllowedCharacters, PackageName}; !Matcher.FindNext())
	{
		InContext.AddMessage(UE::AssetValidation::CreateTokenMessage(
			EMessageSeverity::Error, InAssetData,
			LOCTEXT("AssetNamingConvention_NonASCIICharacters", "Asset name contains non-ASCII characters or illegal symbols."))
		);
		Result &= EDataValidationResult::Invalid;
	}
	
	return Result;
}

#undef LOCTEXT_NAMESPACE
