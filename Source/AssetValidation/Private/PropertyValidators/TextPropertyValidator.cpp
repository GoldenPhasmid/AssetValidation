#include "PropertyValidators/TextPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

UTextPropertyValidator::UTextPropertyValidator()
{
	Descriptor = FTextProperty::StaticClass();
}

void UTextPropertyValidator::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	const FText* TextPtr = GetPropertyValuePtr<FTextProperty>(PropertyMemory, Property);
	check(TextPtr);

	ValidationContext.FailOnCondition(TextPtr->IsEmpty(), Property, NSLOCTEXT("AssetValidation", "TextProperty", "Text property is not set."));
}
