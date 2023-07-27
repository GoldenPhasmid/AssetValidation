#include "PropertyValidators/TextPropertyValidator.h"

#include "AssetValidationStatics.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

bool UTextPropertyValidator::CanValidateProperty(FProperty* Property) const
{
	return AssetValidationStatics::CanValidateProperty(Property) && Property->IsA<FTextProperty>();
}

bool UTextPropertyValidator::CanValidatePropertyValue(FProperty* ParentProperty, FProperty* ValueProperty) const
{
	return AssetValidationStatics::CanValidateProperty(ParentProperty) && ValueProperty->IsA<FTextProperty>();
}

void UTextPropertyValidator::ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationResult& OutValidationResult) const
{
	FText* TextPtr = Property->ContainerPtrToValuePtr<FText>(BasePointer);
	check(TextPtr);

	if (TextPtr->IsEmpty())
	{
		OutValidationResult.PropertyFails(Property, LOCTEXT("AssetValidation_TextProperty", "Text property is not set"));
	}
}

void UTextPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationResult& OutValidationResult) const
{
	FText* TextPtr = static_cast<FText*>(Value);
	check(TextPtr);

	if (TextPtr->IsEmpty())
	{
		OutValidationResult.PropertyFails(ParentProperty, LOCTEXT("AssetValidation_TextPropertyValue", "Text value is not set"));
	}
}

#undef LOCTEXT_NAMESPACE
