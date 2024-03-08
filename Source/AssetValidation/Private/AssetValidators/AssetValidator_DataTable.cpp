#include "AssetValidators/AssetValidator_DataTable.h"

#include "PropertyValidatorSubsystem.h"
#include "PropertyValidators/PropertyValidation.h"

bool UAssetValidator_DataTable::CanValidateAsset_Implementation(UObject* InAsset) const
{
	return Super::CanValidateAsset_Implementation(InAsset) && InAsset->IsAsset() && InAsset->IsA(UDataTable::StaticClass());
}

EDataValidationResult UAssetValidator_DataTable::ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors)
{
	UPropertyValidatorSubsystem* PropertyValidators = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	check(PropertyValidators);
	
	UDataTable* DataTable = CastChecked<UDataTable>(InAsset);
	const UScriptStruct* RowStruct = DataTable->GetRowStruct();

	for (auto& [Name, StructData]: DataTable->GetRowMap())
	{
		FPropertyValidationResult Result = PropertyValidators->IsPropertyContainerValid(DataTable, RowStruct, StructData);
		for (const FText& Text: Result.Warnings)
		{
			AssetWarning(DataTable, Text);
		}
		
		if (Result.ValidationResult == EDataValidationResult::Invalid)
		{
			check(Result.Errors.Num() > 0);
			for (const FText& Text: Result.Errors)
			{
				AssetFails(DataTable, Text, ValidationErrors);
			}
		}
	}

	if (GetValidationResult() == EDataValidationResult::NotValidated)
	{
		AssetPasses(DataTable);
	}

	return GetValidationResult();
}
