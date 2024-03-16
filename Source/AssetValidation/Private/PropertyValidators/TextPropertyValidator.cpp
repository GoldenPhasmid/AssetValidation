#include "PropertyValidators/TextPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

UTextPropertyValidator::UTextPropertyValidator()
{
	PropertyClass = FTextProperty::StaticClass();
}

const FText EmptyTextFailure = NSLOCTEXT("AssetValidation", "TextProperty", "Text property is not set");

void UTextPropertyValidator::ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FText* TextPtr = Property->ContainerPtrToValuePtr<FText>(Container);
	check(TextPtr);

	ValidationContext.FailOnCondition(TextPtr->IsEmpty(), Property, EmptyTextFailure, Property->GetDisplayNameText());
}

void UTextPropertyValidator::ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	const FText* TextPtr = static_cast<const FText*>(Value);
	check(TextPtr);

	ValidationContext.FailOnCondition(TextPtr->IsEmpty(), ParentProperty, EmptyTextFailure);
}
