#include "AssetValidators/AssetValidator_DataTable.h"

#include "AssetValidationDefines.h"
#include "PropertyValidatorSubsystem.h"

bool UAssetValidator_DataTable::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	return Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext) && InObject->IsAsset() && InObject->IsA<UDataTable>();
}

EDataValidationResult UAssetValidator_DataTable::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_DataTable, AssetValidationChannel);
	
	UPropertyValidatorSubsystem* PropertyValidators = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	check(PropertyValidators);
	
	UDataTable* DataTable = CastChecked<UDataTable>(InAsset);
	const UScriptStruct* RowStruct = DataTable->GetRowStruct();

	for (auto& [Name, StructData]: DataTable->GetRowMap())
	{
		FPropertyValidationResult Result = PropertyValidators->ValidateStruct(DataTable, RowStruct, StructData);
		for (const FText& Text: Result.Warnings)
		{
			AssetWarning(DataTable, Text);
		}
		
		if (Result.ValidationResult == EDataValidationResult::Invalid)
		{
			check(Result.Errors.Num() > 0);
			for (const FText& Text: Result.Errors)
			{
				AssetFails(DataTable, Text);
			}
		}
	}

	if (GetValidationResult() == EDataValidationResult::NotValidated)
	{
		AssetPasses(DataTable);
	}

	return GetValidationResult();
}

