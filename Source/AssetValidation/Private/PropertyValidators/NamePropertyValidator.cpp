#include "PropertyValidators/NamePropertyValidator.h"

#include "AssetValidationStatics.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

bool UNamePropertyValidator::CanValidateProperty(FProperty* Property) const
{
	return AssetValidationStatics::CanValidateProperty(Property) && Property->IsA<FNameProperty>();
}

bool UNamePropertyValidator::CanValidatePropertyValue(FProperty* ParentProperty, FProperty* ValueProperty) const
{
	return AssetValidationStatics::CanValidateProperty(ParentProperty) && ValueProperty->IsA<FNameProperty>();
}

void UNamePropertyValidator::ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationResult& OutValidationResult) const
{
	FName* Name = Property->ContainerPtrToValuePtr<FName>(BasePointer);
	check(Name);

	if (*Name == NAME_None)
	{
		OutValidationResult.PropertyFails(Property, LOCTEXT("AssetValidation_NameProperty", "Name property not set"));
	}
}

void UNamePropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationResult& OutValidationResult) const
{
	FName* Name = static_cast<FName*>(Value);
	check(Name);

	if (*Name == NAME_None)
	{
		OutValidationResult.PropertyFails(ValueProperty, LOCTEXT("AssetValidation_NamePropertyValue", "Name value not set"));
	}
}

#undef LOCTEXT_NAMESPACE
