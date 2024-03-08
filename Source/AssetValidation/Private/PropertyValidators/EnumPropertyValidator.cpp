#include "PropertyValidators/EnumPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

UEnumPropertyValidator::UEnumPropertyValidator()
{
	PropertyClass = FEnumProperty::StaticClass();
}

void UEnumPropertyValidator::ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const uint8* ValuePtr = Property->ContainerPtrToValuePtr<uint8>(Container);
	check(ValuePtr);

	if (*ValuePtr == 0)
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_EnumProperty", "Enum property not set"), Property->GetDisplayNameText());
	}
}

void UEnumPropertyValidator::ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	const uint8* ValuePtr = static_cast<const uint8*>(Value);
	check(ValuePtr);

	if (*ValuePtr == 0)
	{
		ValidationContext.PropertyFails(ParentProperty, LOCTEXT("AssetValidation_EnumPropertyValue", "Enum value not set"));
	}
}

#undef LOCTEXT_NAMESPACE