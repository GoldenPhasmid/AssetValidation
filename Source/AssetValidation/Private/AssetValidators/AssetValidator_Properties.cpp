#include "AssetValidators/AssetValidator_Properties.h"

#include "PropertyValidatorSubsystem.h"
#include "PropertyValidators/PropertyValidatorBase.h"

bool UAssetValidator_Properties::CanValidateAsset_Implementation(UObject* InAsset) const
{
	return Super::CanValidateAsset_Implementation(InAsset) && InAsset && InAsset->IsA(UBlueprint::StaticClass());
}

EDataValidationResult UAssetValidator_Properties::ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors)
{
	UPropertyValidatorSubsystem* PropertyValidators = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	check(PropertyValidators);

	FPropertyValidationResult PropertyValidation;
	for (TFieldIterator<FProperty> It(InAsset->GetClass(), EFieldIterationFlags::Default); It; ++It)
	{
		FProperty* Property = *It;
		PropertyValidation.Append(PropertyValidators->IsPropertyValid(InAsset, Property));
	}

	if (PropertyValidation.ValidationResult == EDataValidationResult::Invalid)
	{
		for (const FText& Text: PropertyValidation.ValidationErrors)
		{
			AssetFails(InAsset, Text, ValidationErrors);
		}
	}
	else
	{
		AssetPasses(InAsset);
	}
	
	return PropertyValidation.ValidationResult;
}
