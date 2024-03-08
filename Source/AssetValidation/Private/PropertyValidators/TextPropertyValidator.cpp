#include "PropertyValidators/TextPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

UTextPropertyValidator::UTextPropertyValidator()
{
	PropertyClass = FTextProperty::StaticClass();
}

void UTextPropertyValidator::ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FText* TextPtr = Property->ContainerPtrToValuePtr<FText>(Container);
	check(TextPtr);

	if (TextPtr->IsEmpty())
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_TextProperty", "Text property is not set"), Property->GetDisplayNameText());
	}
}

void UTextPropertyValidator::ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	const FText* TextPtr = static_cast<const FText*>(Value);
	check(TextPtr);

	if (TextPtr->IsEmpty())
	{
		ValidationContext.PropertyFails(ParentProperty, LOCTEXT("AssetValidation_TextPropertyValue", "Text value is not set"));
	}
}

#undef LOCTEXT_NAMESPACE
