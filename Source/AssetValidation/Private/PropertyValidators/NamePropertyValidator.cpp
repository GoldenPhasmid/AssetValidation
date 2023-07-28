#include "PropertyValidators/NamePropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

UNamePropertyValidator::UNamePropertyValidator()
{
	PropertyClass = FNameProperty::StaticClass();
}

void UNamePropertyValidator::ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationContext& ValidationContext) const
{
	const FName* Name = Property->ContainerPtrToValuePtr<FName>(BasePointer);
	check(Name);

	if (*Name == NAME_None)
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_NameProperty", "Name property not set"), Property->GetDisplayNameText());
	}
}

void UNamePropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	const FName* Name = static_cast<FName*>(Value);
	check(Name);

	if (*Name == NAME_None)
	{
		ValidationContext.PropertyFails(ValueProperty, LOCTEXT("AssetValidation_NamePropertyValue", "Name value not set"));
	}
}

#undef LOCTEXT_NAMESPACE
