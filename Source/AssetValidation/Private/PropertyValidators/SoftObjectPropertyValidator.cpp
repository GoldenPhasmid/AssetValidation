#include "PropertyValidators/SoftObjectPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

USoftObjectPropertyValidator::USoftObjectPropertyValidator()
{
	PropertyClass = FSoftObjectProperty::StaticClass();
}

void USoftObjectPropertyValidator::ValidateProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FSoftObjectPtr* SoftObjectPtr = GetPropertyValuePtr<FSoftObjectProperty>(PropertyMemory, Property);
	check(SoftObjectPtr);

	// @todo: load soft object to verify it?
	ValidationContext.FailOnCondition(SoftObjectPtr->IsNull(), Property, NSLOCTEXT("AssetValidation", "SoftObjectProperty", "Soft object property not set"));
}
