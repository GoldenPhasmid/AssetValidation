#include "PropertyValidators/EnumPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

UEnumPropertyValidator::UEnumPropertyValidator()
{
	Descriptor = FEnumProperty::StaticClass();
}

void UEnumPropertyValidator::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	const uint8* ValuePtr = static_cast<const uint8*>(PropertyMemory);
	check(ValuePtr);

	ValidationContext.FailOnCondition(*ValuePtr == 0, Property, NSLOCTEXT("AssetValidation", "EnumProperty", "Enum property not set."));
}
