#include "PropertyValidators/SoftObjectPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

USoftObjectPropertyValidator::USoftObjectPropertyValidator()
{
	PropertyClass = FSoftObjectProperty::StaticClass();
}

void USoftObjectPropertyValidator::ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationContext& ValidationContext) const
{
	FSoftObjectProperty* SoftObjectProperty = CastFieldChecked<FSoftObjectProperty>(Property);
	FSoftObjectPtr* SoftObjectPtr = SoftObjectProperty->ContainerPtrToValuePtr<FSoftObjectPtr>(BasePointer);
	check(SoftObjectPtr);
		
	if (SoftObjectPtr->IsNull()) 
	{
		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_SoftObjectProperty", "Soft object property not set"), Property->GetDisplayNameText());
	}
	//@todo: load soft object to verify it?
}

void USoftObjectPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	FSoftObjectPtr* SoftObjectPtr = static_cast<FSoftObjectPtr*>(Value);
	check(SoftObjectPtr);

	if (SoftObjectPtr->IsNull())
	{
		ValidationContext.PropertyFails(ParentProperty, LOCTEXT("AssetValidation_SoftObjectPropertyValue", "Soft object value not set"));
	}
}

#undef LOCTEXT_NAMESPACE
