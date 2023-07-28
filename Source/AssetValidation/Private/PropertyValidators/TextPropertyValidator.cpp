#include "PropertyValidators/TextPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

UTextPropertyValidator::UTextPropertyValidator()
{
	PropertyClass = FTextProperty::StaticClass();
}

void UTextPropertyValidator::ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationContext& ValidationContext) const
{
	const FText* TextPtr = Property->ContainerPtrToValuePtr<FText>(BasePointer);
	check(TextPtr);

	if (TextPtr->IsEmpty())
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_TextProperty", "Text property is not set"), Property->GetDisplayNameText());
	}
}

void UTextPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	const FText* TextPtr = static_cast<FText*>(Value);
	check(TextPtr);

	if (TextPtr->IsEmpty())
	{
		ValidationContext.PropertyFails(ParentProperty, LOCTEXT("AssetValidation_TextPropertyValue", "Text value is not set"));
	}
}

#undef LOCTEXT_NAMESPACE
