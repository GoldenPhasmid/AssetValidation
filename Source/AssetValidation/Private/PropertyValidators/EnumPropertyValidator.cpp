#include "PropertyValidators/EnumPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

UEnumPropertyValidator::UEnumPropertyValidator()
{
	PropertyClass = FEnumProperty::StaticClass();
}

const FText EmptyEnumFailure = NSLOCTEXT("AssetValidation", "EnumProperty", "Enum property not set");

void UEnumPropertyValidator::ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const uint8* ValuePtr = Property->ContainerPtrToValuePtr<uint8>(Container);
	check(ValuePtr);

	ValidationContext.FailOnCondition(*ValuePtr == 0, Property, EmptyEnumFailure, Property->GetDisplayNameText());
}

void UEnumPropertyValidator::ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	const uint8* ValuePtr = static_cast<const uint8*>(Value);
	check(ValuePtr);

	ValidationContext.FailOnCondition(*ValuePtr == 0, ParentProperty, EmptyEnumFailure);
}
