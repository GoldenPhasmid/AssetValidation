#include "AssetValidators/AssetValidator_Properties.h"

#include "PropertyValidatorSubsystem.h"
#include "PropertyValidators/PropertyValidation.h"

UAssetValidator_Properties::UAssetValidator_Properties()
{
	
}

bool UAssetValidator_Properties::CanValidateAsset_Implementation(UObject* InAsset) const
{
	if (!Super::CanValidateAsset_Implementation(InAsset))
	{
		return false;
	};

	return InAsset && InAsset->IsA(UBlueprint::StaticClass());
}

EDataValidationResult UAssetValidator_Properties::ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors)
{
	UPropertyValidatorSubsystem* PropertyValidators = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	check(PropertyValidators);
	
	UClass* Class = CastChecked<UBlueprint>(InAsset)->GeneratedClass;
	UObject* Object = Class->GetDefaultObject();
	check(Class && Object);
	
	FPropertyValidationResult Result = PropertyValidators->IsPropertyContainerValid(Object);
	
	if (Result.ValidationResult == EDataValidationResult::Invalid)
	{
		for (const FText& Text: Result.Errors)
		{
			AssetFails(Object, Text, ValidationErrors);
		}
		for (const FText& Text: Result.Warnings)
		{
			AssetWarning(Object, Text);
		}
	}
	else
	{
		AssetPasses(Object);
	}
	
	return Result.ValidationResult;
}

bool UAssetValidator_Properties::CanValidateClass(UClass* Class) const
{
	if (IsBlueprintGeneratedClass(Class))
	{
		return true;
	}
	
	const UPackage* Package = Class->GetPackage();
	const FString PackageName = Package->GetName();

#if WITH_EDITOR
	if (PackageName.StartsWith("/Script/AssetValidation"))
	{
		return true;
	}
#endif
	
	return PackagesToValidate.ContainsByPredicate([PackageName](const FString& ModulePath)
	{
		return PackageName.StartsWith(ModulePath);
	});
}

bool UAssetValidator_Properties::IsBlueprintGeneratedClass(UClass* Class) const
{
	const FString PackageName = Class->GetPackage()->GetName();
	return PackageName.StartsWith(TEXT("/Game/"));
}