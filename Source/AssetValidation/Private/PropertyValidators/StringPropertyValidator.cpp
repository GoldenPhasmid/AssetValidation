#include "PropertyValidators/StringPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"


UStringPropertyValidator::UStringPropertyValidator()
{
	PropertyClass = FStrProperty::StaticClass();
}

void UStringPropertyValidator::ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationContext& ValidationContext) const
{
	const FString* Str = Property->ContainerPtrToValuePtr<FString>(BasePointer);
	check(Str);

	if (Str->IsEmpty())
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_StrProperty", "String property not set"));
	}
}

void UStringPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	const FString* Str = ValueProperty->ContainerPtrToValuePtr<FString>(Value);
	check(Str);

	if (Str->IsEmpty())
	{
		ValidationContext.PropertyFails(ValueProperty, LOCTEXT("AssetValidation_StrPropertyValue", "String value not set"));
	}
}

#undef LOCTEXT_NAMESPACE
