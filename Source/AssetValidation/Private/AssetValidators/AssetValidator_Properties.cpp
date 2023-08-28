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
	for (const FText& Text: Result.Warnings)
	{
		AssetWarning(Object, Text);
	}
	
	if (Result.ValidationResult == EDataValidationResult::Invalid)
	{
		check(Result.Errors.Num() > 0);
		for (const FText& Text: Result.Errors)
		{
			AssetFails(Object, Text, ValidationErrors);
		}
	}
	else
	{
		AssetPasses(Object);
	}
	
	return Result.ValidationResult;
}