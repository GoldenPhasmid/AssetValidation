#include "PropertyValidators/ObjectPropertyValidator.h"

#include "PropertyValidatorSubsystem.h"
#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

UObjectPropertyValidator::UObjectPropertyValidator()
{
	PropertyClass = FObjectProperty::StaticClass();
}

bool UObjectPropertyValidator::CanValidateProperty(const FProperty* Property) const
{
	return Super::CanValidateProperty(Property) || Property->HasMetaData(ValidationNames::ValidateRecursive);
}

void UObjectPropertyValidator::ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	UObject* const* ObjectPtr = Property->ContainerPtrToValuePtr<UObject*>(Container);
    check(ObjectPtr);

	UObject* Object = *ObjectPtr;
	const bool bObjectValid = Object != nullptr && Object->IsValidLowLevel();
    if (!bObjectValid)
    {
    	// determine whether object value should be validated
    	if (Super::CanValidateProperty(Property))
    	{
    		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_ObjectProperty", "Object property not set"), Property->GetDisplayNameText());
    	}
    	return;
    }
	
    if (Property->HasMetaData(ValidationNames::ValidateRecursive))
    {
    	ValidationContext.PushPrefix(Property->GetName() + "." + Object->GetName());

    	// validate underlying object recursively
    	ValidationContext.IsPropertyContainerValid(Object, Object->GetClass());

    	ValidationContext.PopPrefix();
    }
}

void UObjectPropertyValidator::ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	UObject* Object = *static_cast<UObject* const*>(Value);
	
	const bool bObjectValid = Object != nullptr && Object->IsValidLowLevel();
	if (!bObjectValid)
	{
		ValidationContext.PropertyFails(ParentProperty, LOCTEXT("AssetValidation_ObjectProperty", "Object value not set"));
		return;
	}
	
	if (ParentProperty->HasMetaData(ValidationNames::ValidateRecursive))
	{
		ValidationContext.PushPrefix(Object->GetName());
		// validate underlying object recursively
		ValidationContext.IsPropertyContainerValid(Object, Object->GetClass());

		ValidationContext.PopPrefix();
	}
}

#undef LOCTEXT_NAMESPACE
