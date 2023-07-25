#include "PropertyValidators/ArrayPropertyValidator.h"

#include "AssetValidationStatics.h"

bool UArrayPropertyValidator::CanValidateProperty(FProperty* Property) const
{
	return AssetValidationStatics::CanValidateProperty(Property) && Property->IsA<FArrayProperty>();
}

bool UArrayPropertyValidator::CanValidatePropertyValue(FProperty* ParentProperty, FProperty* ValueProperty) const
{
	return AssetValidationStatics::CanValidateProperty(ValueProperty) && ValueProperty->IsA<FArrayProperty>();
}

void UArrayPropertyValidator::ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationResult& OutValidationResult) const
{
	
}

void UArrayPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationResult& OutValidationResult) const
{
	
}
