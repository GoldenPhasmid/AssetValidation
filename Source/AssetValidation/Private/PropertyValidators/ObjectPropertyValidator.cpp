
#include "PropertyValidators/ObjectPropertyValidator.h"

#include "AssetValidationStatics.h"

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
	FObjectProperty* ObjectProperty = CastFieldChecked<FObjectProperty>(Property);
	UObject** ObjectPtr = ObjectProperty->ContainerPtrToValuePtr<UObject*>(BasePointer);
    check(ObjectPtr);
    
    if (*ObjectPtr == nullptr || !(*ObjectPtr)->IsValidLowLevel())
    {
    	OutValidationResult.PropertyFails(Property);
    }
    else
    {
    	OutValidationResult.PropertyPasses(Property);
    }
}

void UObjectPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationResult& OutValidationResult) const
{
	FObjectProperty* ObjectProperty = CastFieldChecked<FObjectProperty>(ValueProperty);
	UObject* Object = *static_cast<UObject**>(Value);
	
	if (Object == nullptr || !Object->IsValidLowLevel())
	{
		OutValidationResult.PropertyFails(ParentProperty);
	}
	else
	{
		OutValidationResult.PropertyPasses(ParentProperty);
	}
}
