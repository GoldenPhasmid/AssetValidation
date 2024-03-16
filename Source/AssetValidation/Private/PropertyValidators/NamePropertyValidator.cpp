#include "PropertyValidators/NamePropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

UNamePropertyValidator::UNamePropertyValidator()
{
	PropertyClass = FNameProperty::StaticClass();
}

void UNamePropertyValidator::ValidateProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FName* Name = GetPropertyValuePtr<FNameProperty>(PropertyMemory, Property);
	check(Name);

	ValidationContext.FailOnCondition(*Name == NAME_None, Property, NSLOCTEXT("AssetValidation", "NameProperty", "Name property not set"));
}

