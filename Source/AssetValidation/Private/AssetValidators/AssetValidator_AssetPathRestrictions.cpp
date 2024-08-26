#include "AssetValidators/AssetValidator_AssetPathRestrictions.h"

#include "AssetReferencingDomains.h"
#include "AssetReferencingPolicySettings.h"
#include "AssetReferencingPolicySubsystem.h"
#include "AssetValidationDefines.h"
#include "AssetValidationStatics.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

bool FAssetPathDescription::PassesPathDescription(const FAssetData& AssetData) const
{
	if (IsEmpty())
	{
		return true;
	}
	
	const UAssetReferencingPolicySubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetReferencingPolicySubsystem>();
	check(Subsystem);

	const FString PackagePath = AssetData.PackagePath.ToString();
	for (const FString& DomainName: AssetDomains)
	{
		TSharedPtr<FDomainData> AssetDomain = Subsystem->GetDomainDB()->FindOrAddDomainByName(DomainName);
		if (AssetDomain->IsValid())
		{
			for (const FString& RootPath: AssetDomain->DomainRootPaths)
			{
				if (PackagePath.StartsWith(RootPath))
				{
					return true;
				}
			}
		}
	}

	for (const FString& RegexPath: RegexAssetPaths)
	{
		FRegexPattern Pattern{RegexPath};
		if (FRegexMatcher Matcher{Pattern, PackagePath}; Matcher.FindNext())
		{
			return true;
		}
	}

	return false;
}

TArray<FString> UAssetPathPolicySettings::GetDomains()
{
	TArray<FString> Result;
	const UAssetReferencingPolicySettings* RefPolicy = GetDefault<UAssetReferencingPolicySettings>();

	Result.Add(UAssetReferencingPolicySettings::GameDomainName);
	for (const FARPDomainDefinitionByContentRoot& DomainDef : RefPolicy->AdditionalDomains)
	{
		Result.Add(DomainDef.DomainName);
	}
	
	const UAssetReferencingPolicySubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetReferencingPolicySubsystem>();
	check(Subsystem);
	
	Result.Append(Subsystem->GetDomainDB()->GetDomainsDefinedByPlugins());

	return Result;
}

UAssetValidator_AssetPathRestrictions::UAssetValidator_AssetPathRestrictions()
{
	bIsConfigDisabled = false; // enabled by default

	bCanRunParallelMode = true;
	bRequiresLoadedAsset = false;
	bRequiresTopLevelAsset = true;
	bCanValidateActors = false;
}

bool UAssetValidator_AssetPathRestrictions::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	if (!Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext))
	{
		return false;
	}

	return true;
}

EDataValidationResult UAssetValidator_AssetPathRestrictions::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	return ValidateAsset_Implementation(InAssetData, Context);
}

EDataValidationResult UAssetValidator_AssetPathRestrictions::ValidateAsset_Implementation(const FAssetData& InAssetData, FDataValidationContext& InContext)
{
	FSoftClassPath ClassPath{InAssetData.AssetClassPath.ToString()};
	const UAssetPathPolicySettings& PathPolicy = UAssetPathPolicySettings::Get();

	for (const FAssetTypeDomainDescription& AssetTypeDomain: PathPolicy.AssetRules)
	{
		if (AssetTypeDomain.AssetTypes.Contains(ClassPath))
		{
			return ValidateAssetPath(AssetTypeDomain.AllowedPaths, AssetTypeDomain.DisallowedPaths, InAssetData, InContext);
		}
	}

	return ValidateAssetPath(PathPolicy.GlobalAllowedPaths, PathPolicy.GlobalDisallowedPaths, InAssetData, InContext);
}

EDataValidationResult UAssetValidator_AssetPathRestrictions::ValidateAssetPath(const FAssetPathDescription& AllowedPaths, const FAssetPathDescription& DisallowedPaths, const FAssetData& AssetData, FDataValidationContext& Context)
{
	if (AllowedPaths.PassesPathDescription(AssetData) && !DisallowedPaths.PassesPathDescription(AssetData))
	{
		return EDataValidationResult::Valid;
	}

	// @todo: depending on the asset status give a different error message
	UE::AssetValidation::AddTokenMessage(Context, EMessageSeverity::Error,
		AssetData, LOCTEXT("AssetPathRestriction_Failed", "Edited asset is located outside of restricted paths. See Project Settings -> Asset Path Policy for more information.")
	);
	return EDataValidationResult::Invalid;
}

#undef LOCTEXT_NAMESPACE