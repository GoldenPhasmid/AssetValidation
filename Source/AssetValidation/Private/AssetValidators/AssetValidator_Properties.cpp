#include "AssetValidators/AssetValidator_Properties.h"

#include "PropertyValidatorSubsystem.h"
#include "Engine/UserDefinedEnum.h"
#include "Engine/UserDefinedStruct.h"

EDataValidationResult UAssetValidator_Properties::ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors)
{
	UPropertyValidatorSubsystem* PropertyValidators = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	check(PropertyValidators);

	UClass* Class = InAsset->GetClass();
	UObject* Object = InAsset;
	
	if (Object->IsA<UUserDefinedStruct>() || Object->IsA<UUserDefinedEnum>())
	{
		// ignore user defined struct and enum blueprints
		return EDataValidationResult::Valid;
	}
	
	if (UBlueprint* Blueprint = Cast<UBlueprint>(InAsset))
	{
		Class = Blueprint->GeneratedClass;
		Object = Class->GetDefaultObject();
	}
	
	check(Class && Object);
	
	FPropertyValidationResult Result = PropertyValidators->ValidateObject(Object);
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