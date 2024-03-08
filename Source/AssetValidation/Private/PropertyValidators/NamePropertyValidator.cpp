#include "PropertyValidators/NamePropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

UNamePropertyValidator::UNamePropertyValidator()
{
	PropertyClass = FNameProperty::StaticClass();
}

void UNamePropertyValidator::ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FName* Name = Property->ContainerPtrToValuePtr<FName>(Container);
	check(Name);

	if (*Name == NAME_None)
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_NameProperty", "Name property not set"), Property->GetDisplayNameText());
	}
}

void UNamePropertyValidator::ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	const FName* Name = static_cast<const FName*>(Value);
	check(Name);

	if (*Name == NAME_None)
	{
		ValidationContext.PropertyFails(ParentProperty, LOCTEXT("AssetValidation_NamePropertyValue", "Name value not set"));
	}
}

#undef LOCTEXT_NAMESPACE
