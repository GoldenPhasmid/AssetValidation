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

void UObjectPropertyValidator::ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationContext& ValidationContext) const
{
	UObject** ObjectPtr = Property->ContainerPtrToValuePtr<UObject*>(BasePointer);
    check(ObjectPtr);

	UObject* Object = *ObjectPtr;
	const bool bObjectValid = Object != nullptr && Object->IsValidLowLevel();
    if (!bObjectValid)
    {
    	if (Property->HasMetaData(ValidationNames::Validate))
    	{
    		ValidationContext.PropertyFails(Property, LOCTEXT("AssetValidation_ObjectProperty", "Object property not set"));
    	}
    	return;
    }
	
    if (Property->HasMetaData(ValidationNames::ValidateRecursive))
    {
    	// validate underlying object recursively
    	ValidationContext.IsPropertyContainerValid(Object, Object->GetClass());
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
			ValidationContext.PropertyFails(ValueProperty, LOCTEXT("AssetValidation_ObjectProperty", "Object property not set"));
		}
		return;
	}
	
	if (ValueProperty->HasMetaData(ValidationNames::ValidateRecursive))
	{
		// validate underlying object recursively
		ValidationContext.IsPropertyContainerValid(Object, Object->GetClass());
	}
}

#undef LOCTEXT_NAMESPACE
