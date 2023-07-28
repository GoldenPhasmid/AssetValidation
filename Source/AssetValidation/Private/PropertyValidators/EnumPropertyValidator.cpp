#include "PropertyValidators/EnumPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

FFieldClass* UEnumPropertyValidator::GetPropertyClass() const
{
	return FEnumProperty::StaticClass();
}

void UEnumPropertyValidator::ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationContext& ValidationContext) const
{
	const uint8* ValuePtr = Property->ContainerPtrToValuePtr<uint8>(BasePointer);
	check(ValuePtr);

	if (*ValuePtr == 0)
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_EnumProperty", "Enum property not set"), Property->GetDisplayNameText());
	}
}

void UEnumPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	const uint8* ValuePtr = static_cast<uint8*>(Value);
	check(ValuePtr);

	if (*ValuePtr == 0)
	{
		ValidationContext.PropertyFails(ParentProperty, LOCTEXT("AssetValidation_EnumPropertyValue", "Enum value not set"));
	}
}

#undef LOCTEXT_NAMESPACE