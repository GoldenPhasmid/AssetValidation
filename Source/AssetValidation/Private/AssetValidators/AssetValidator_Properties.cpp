#include "AssetValidators/AssetValidator_Properties.h"

#include "PropertyValidatorSubsystem.h"
#include "Engine/UserDefinedEnum.h"
#include "Engine/UserDefinedStruct.h"

#if WITH_DATA_VALIDATION_UPDATE
bool UAssetValidator_Properties::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	return InObject != nullptr && !InObject->IsA<UUserDefinedStruct>() && !InObject->IsA<UUserDefinedEnum>();
}

EDataValidationResult UAssetValidator_Properties::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
#else
EDataValidationResult UAssetValidator_Properties::ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors)
#endif
{
	UPropertyValidatorSubsystem* PropertyValidators = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	check(PropertyValidators);

	UClass* Class = InAsset->GetClass();
	UObject* Object = InAsset;
	
	if (UBlueprint* Blueprint = Cast<UBlueprint>(InAsset))
	{
		Class = Blueprint->GeneratedClass;
		Object = Class->GetDefaultObject();
	}
	
	check(Class && Object);
	
	FPropertyValidationResult Result = PropertyValidators->ValidateObject(Object);
	for (const FText& Warning: Result.Warnings)
	{
		AssetMessage(InAssetData, EMessageSeverity::Warning, Warning);
	}
	
	if (Result.ValidationResult == EDataValidationResult::Invalid)
	{
		check(Result.Errors.Num() > 0);
		for (const FText& Error: Result.Errors)
		{
			AssetMessage(InAssetData, EMessageSeverity::Error, Error);
		}
	}
	else
	{
		AssetPasses(Object);
	}
	
	return Result.ValidationResult;
}
