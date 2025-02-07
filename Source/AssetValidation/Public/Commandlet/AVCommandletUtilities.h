#pragma once

#include "CoreMinimal.h"
#include "PropertyValidators/PropertyValidation.h"

namespace UE::AssetValidation
{
	/**
	 * Transforms array of data of structs T to the csv-format string
	 * PName1,PName2,PName3,...
	 * Value1,Value2,Value3,...
	 * 
	 * @tparam T struct type, auto deduced
	 * @param Data data array
	 * @return csv-formatted string that represents data array
	 */
	template <typename T>
	static FString CsvExport(const TArray<T>& Data)
	{
		FString ExportedText{};
		

		TArray<const UStruct*, TInlineAllocator<4>> StructArray;
		TArray<const FProperty*, TInlineAllocator<16>> PropertyArray;
		for (const UStruct* Struct = TBaseStructure<T>::Get(); Struct; Struct = Struct->GetSuperStruct())
		{
			StructArray.Add(Struct);
		}

		// traverse backwards, from parent to child structs and gather properties, while generating column header string
		for (int32 Index = StructArray.Num() - 1; Index >= 0; --Index)
		{
			for (TFieldIterator<FProperty> It{StructArray[Index], EFieldIterationFlags::None}; It; ++It)
			{
				const FProperty* Property = *It;
				if (UE::AssetValidation::IsContainerProperty(Property))
				{
					continue;
				}
			
				FString ColumnHeader = Property->GetAuthoredName();
				ExportedText += ColumnHeader;
				ExportedText += TEXT(",");
			
				PropertyArray.Add(Property);
			}
		}
	
		if (ExportedText.IsEmpty())
		{
			// no properties?
			return ExportedText;
		}
		// replace last ',' with '\n'
		ExportedText[ExportedText.Len() - 1] = '\n';

		for (const T& Row: Data)
		{
			for (const FProperty* Property: PropertyArray)
			{
				// const uint8* RowData = Property->ContainerPtrToValuePtr<uint8>(&Row, 0);
				const FString PropertyValue = DataTableUtils::GetPropertyValueAsString(Property, reinterpret_cast<const uint8*>(&Row), EDataTableExportFlags::None);
				ExportedText += TEXT("\"");
				ExportedText += PropertyValue.Replace(TEXT("\""), TEXT("\"\""));
				ExportedText += TEXT("\"");
				ExportedText += TEXT(",");
			}

			// replace last ',' with '\n'
			ExportedText[ExportedText.Len() - 1] = '\n';
		}

		return ExportedText;
	}
} // UE::AssetValidation