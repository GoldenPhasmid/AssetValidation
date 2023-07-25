#include "PropertyValidators/SoftObjectPropertyValidator.h"

#include "AssetValidationStatics.h"

bool USoftObjectPropertyValidator::CanValidateProperty(FProperty* Property) const
{
	return AssetValidationStatics::CanValidateProperty(Property) && Property->IsA<FSoftObjectProperty>();
}

bool USoftObjectPropertyValidator::CanValidatePropertyValue(FProperty* ParentProperty, FProperty* ValueProperty) const
{
	return AssetValidationStatics::CanValidateProperty(ValueProperty) && ValueProperty->IsA<FSoftObjectProperty>();
}

void USoftObjectPropertyValidator::ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationResult& OutValidationResult) const
{
	FSoftObjectProperty* SoftObjectProperty = CastFieldChecked<FSoftObjectProperty>(Property);
	FSoftObjectPtr* SoftObjectPtr = SoftObjectProperty->ContainerPtrToValuePtr<FSoftObjectPtr>(BasePointer);
	check(SoftObjectPtr);
		
	if (SoftObjectPtr->IsNull()) 
	{
		OutValidationResult.PropertyFails(Property);
	}
	else
	{
		//@todo: load soft object to verify it?
		OutValidationResult.PropertyPasses(Property);
	}
}

void USoftObjectPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationResult& OutValidationResult) const
{
	FSoftObjectProperty* SoftObjectProperty = CastFieldChecked<FSoftObjectProperty>(ValueProperty);
	FSoftObjectPtr* SoftObjectPtr = static_cast<FSoftObjectPtr*>(Value);
	check(SoftObjectPtr);

	if (SoftObjectPtr->IsNull())
	{
		OutValidationResult.PropertyFails(ParentProperty);
	}
	else
	{
		OutValidationResult.PropertyPasses(ParentProperty);
	}
}
