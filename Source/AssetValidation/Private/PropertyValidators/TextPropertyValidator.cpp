#include "PropertyValidators/TextPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

UTextPropertyValidator::UTextPropertyValidator()
{
	PropertyClass = FTextProperty::StaticClass();
}

void UTextPropertyValidator::ValidateProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FText* TextPtr = GetPropertyValuePtr<FTextProperty>(PropertyMemory, Property);
	check(TextPtr);

	ValidationContext.FailOnCondition(TextPtr->IsEmpty(), Property, NSLOCTEXT("AssetValidation", "TextProperty", "Text property is not set"));
}
