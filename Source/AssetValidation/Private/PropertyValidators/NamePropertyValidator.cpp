#include "PropertyValidators/NamePropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

UNamePropertyValidator::UNamePropertyValidator()
{
	Descriptor = FNameProperty::StaticClass();
}

void UNamePropertyValidator::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	const FName* Name = GetPropertyValuePtr<FNameProperty>(PropertyMemory, Property);
	check(Name);

	ValidationContext.FailOnCondition(*Name == NAME_None, Property, NSLOCTEXT("AssetValidation", "NameProperty", "Name property not set."));
}

