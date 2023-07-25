#include "AssetValidators/AssetValidator_Properties.h"

#include "PropertyValidatorSubsystem.h"
#include "PropertyValidators/PropertyValidatorBase.h"

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

	FPropertyValidationResult PropertyValidation;
	
	UClass* Class = CastChecked<UBlueprint>(InAsset)->GeneratedClass;
	UObject* Object = Class->GetDefaultObject();
	check(Class && Object);
	
	while (Class && CanValidateClass(Class))
	{
		if (bSkipBlueprintGeneratedClasses && IsBlueprintGeneratedClass(Class))
		{
			Class = Class->GetSuperClass();
			continue;
		}
		
		for (TFieldIterator<FProperty> It(Class, EFieldIterationFlags::None); It; ++It)
		{
			FProperty* Property = *It;
			// do not validate transient or deprecated properties
			// only validate properties that we can actually edit in editor
			if (!Property->HasAnyPropertyFlags(EPropertyFlags::CPF_Transient) && Property->HasAnyPropertyFlags(EPropertyFlags::CPF_Edit))
			{
				PropertyValidation.Append(PropertyValidators->IsPropertyValid(Object, Property));
			}
		}
		
		Class = Class->GetSuperClass();
	}
	
	if (PropertyValidation.ValidationResult == EDataValidationResult::Invalid)
	{
		for (const FText& Text: PropertyValidation.ValidationErrors)
		{
			AssetFails(Object, Text, ValidationErrors);
		}
	}
	else
	{
		AssetPasses(Object);
	}
	
	return PropertyValidation.ValidationResult;
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
