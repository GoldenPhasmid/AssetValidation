#include "PropertyValidators/EnumPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

UEnumPropertyValidator::UEnumPropertyValidator()
{
	PropertyClass = FEnumProperty::StaticClass();
}

void UEnumPropertyValidator::ValidateProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const uint8* ValuePtr = static_cast<const uint8*>(PropertyMemory);
	check(ValuePtr);

	ValidationContext.FailOnCondition(*ValuePtr == 0, Property, NSLOCTEXT("AssetValidation", "EnumProperty", "Enum property not set"));
}
