#include "PropertyValidators/ObjectPropertyValidator.h"

#include "PropertyValidatorSubsystem.h"
#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

UObjectPropertyValidator::UObjectPropertyValidator()
{
	PropertyClass = FObjectProperty::StaticClass();
}

bool UObjectPropertyValidator::CanValidateProperty(FProperty* Property) const
{
	return Super::CanValidateProperty(Property) || Property->HasMetaData(ValidationNames::ValidateRecursive);
}

void UObjectPropertyValidator::ValidateProperty(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	UObject** ObjectPtr = Property->ContainerPtrToValuePtr<UObject*>(Container);
    check(ObjectPtr);

	UObject* Object = *ObjectPtr;
	const bool bObjectValid = Object != nullptr && Object->IsValidLowLevel();
    if (!bObjectValid)
    {
    	if (Property->HasMetaData(ValidationNames::Validate))
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

void UObjectPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	UObject* Object = *static_cast<UObject**>(Value);
	
	const bool bObjectValid = Object != nullptr && Object->IsValidLowLevel();
	if (!bObjectValid)
	{
		if (ValueProperty->HasMetaData(ValidationNames::Validate))
		{
			ValidationContext.PropertyFails(ValueProperty, LOCTEXT("AssetValidation_ObjectProperty", "Object value not set"));
		}
		return;
	}
	
	if (ValueProperty->HasMetaData(ValidationNames::ValidateRecursive))
	{
		ValidationContext.PushPrefix(Object->GetName());
		// validate underlying object recursively
		ValidationContext.IsPropertyContainerValid(Object, Object->GetClass());

		ValidationContext.PopPrefix();
	}
}

#undef LOCTEXT_NAMESPACE
