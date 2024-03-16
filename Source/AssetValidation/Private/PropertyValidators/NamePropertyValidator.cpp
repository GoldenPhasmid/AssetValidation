#include "PropertyValidators/NamePropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

UNamePropertyValidator::UNamePropertyValidator()
{
	PropertyClass = FNameProperty::StaticClass();
}

const FText EmptyNameFailure = NSLOCTEXT("AssetValidation", "NameProperty", "Name property not set");

void UNamePropertyValidator::ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FName* Name = Property->ContainerPtrToValuePtr<FName>(Container);
	check(Name);

	ValidationContext.FailOnCondition(*Name == NAME_None, Property, EmptyNameFailure, Property->GetDisplayNameText());
}

void UNamePropertyValidator::ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	const FName* Name = static_cast<const FName*>(Value);
	check(Name);

	ValidationContext.FailOnCondition(*Name == NAME_None, ParentProperty, EmptyNameFailure);
}

