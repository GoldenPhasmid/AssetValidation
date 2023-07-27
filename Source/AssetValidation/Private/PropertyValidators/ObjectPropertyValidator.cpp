#include "PropertyValidators/ObjectPropertyValidator.h"

#include "AssetValidationStatics.h"
#include "PropertyValidatorSubsystem.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

bool UObjectPropertyValidator::CanValidateProperty(FProperty* Property) const
{
	return AssetValidationStatics::CanValidateProperty(Property) && Property->IsA<FObjectProperty>();
}

bool UObjectPropertyValidator::CanValidatePropertyValue(FProperty* ParentProperty, FProperty* ValueProperty) const
{
	return AssetValidationStatics::CanValidateProperty(ValueProperty) && ValueProperty->IsA<FObjectProperty>();
}

void UObjectPropertyValidator::ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationResult& OutValidationResult) const
{
	UObject** ObjectPtr = Property->ContainerPtrToValuePtr<UObject*>(BasePointer);
    check(ObjectPtr);

	UObject* Object = *ObjectPtr;
    if (Object == nullptr || !Object->IsValidLowLevel())
    {
    	OutValidationResult.PropertyFails(Property, LOCTEXT("AssetValidation_ObjectProperty", "Object property not set"));
    }
    else
    {
    	OutValidationResult.PropertyPasses(Property);

    	// validate underlying object recursively
    	if (Property->HasMetaData(ValidationNames::ValidateRecursive))
    	{
    		UPropertyValidatorSubsystem* PropertyValidators = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
    		check(PropertyValidators);

    		PropertyValidators->IsPropertyContainerValid(Object, Object->GetClass(), OutValidationResult);
    	}
    }
}

void UObjectPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationResult& OutValidationResult) const
{
	UObject* Object = *static_cast<UObject**>(Value);
	
	if (Object == nullptr || !Object->IsValidLowLevel())
	{
		OutValidationResult.PropertyFails(ParentProperty, LOCTEXT("AssetValidation_ObjectPropertyValue", "Object value not set"));
	}
	else
	{
		OutValidationResult.PropertyPasses(ParentProperty);

		// ValueProperty is not a uproperty but an underlying property of a ParentProperty
		if (ParentProperty->HasMetaData(ValidationNames::ValidateRecursive))
		{
			UPropertyValidatorSubsystem* PropertyValidators = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
			check(PropertyValidators);

			PropertyValidators->IsPropertyContainerValid(Object, Object->GetClass(), OutValidationResult);
		}
	}
}

#undef LOCTEXT_NAMESPACE
