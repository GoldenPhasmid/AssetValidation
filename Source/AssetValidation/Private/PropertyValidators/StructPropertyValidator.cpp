#include "PropertyValidators/StructPropertyValidator.h"

#include "AssetValidationStatics.h"

bool UStructPropertyValidator::CanValidateProperty(FProperty* Property) const
{
	return AssetValidationStatics::CanValidateProperty(Property) && Property->IsA<FStructProperty>();
}

bool UStructPropertyValidator::CanValidatePropertyValue(FProperty* ParentProperty, FProperty* ValueProperty) const
{
	return AssetValidationStatics::CanValidateProperty(ValueProperty) && ValueProperty->IsA<FStructProperty>();
}

void UStructPropertyValidator::ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationResult& OutValidationResult) const
{
	FStructProperty* StructProperty = CastFieldChecked<FStructProperty>(Property);

	UScriptStruct* Struct = StructProperty->Struct;
	for (TFieldIterator<FProperty> It(Struct, EFieldIterationFlags::IncludeSuper); It; ++It)
	{
		// @todo: recursive validation
	}
}

void UStructPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationResult& OutValidationResult) const
{
	// @todo: value validation?
}
