#include "PropertyValidators/StringPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"


UStringPropertyValidator::UStringPropertyValidator()
{
	PropertyClass = FStrProperty::StaticClass();
}

void UStringPropertyValidator::ValidateProperty(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FString* Str = Property->ContainerPtrToValuePtr<FString>(Container);
	check(Str);

	if (Str->IsEmpty())
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_StrProperty", "String property not set"), Property->GetDisplayNameText());
	}
}

void UStringPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	const FString* Str = ValueProperty->ContainerPtrToValuePtr<FString>(Value);
	check(Str);

	if (Str->IsEmpty())
	{
		ValidationContext.PropertyFails(ParentProperty, LOCTEXT("AssetValidation_StrPropertyValue", "String value not set"));
	}
}

#undef LOCTEXT_NAMESPACE
