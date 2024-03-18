#include "PropertyValidators/ObjectPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

UObjectPropertyValidator::UObjectPropertyValidator()
{
	PropertyClass = FObjectProperty::StaticClass();
}

void UObjectPropertyValidator::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const TObjectPtr<UObject>* ObjectPtr = GetPropertyValuePtr<FObjectProperty>(PropertyMemory, Property);
	check(ObjectPtr);

	const UObject* Object = ObjectPtr->Get();
	const bool bObjectValid = Object != nullptr && Object->IsValidLowLevel();
	ValidationContext.FailOnCondition(!bObjectValid, Property, NSLOCTEXT("AssetValidation", "ObjectProperty", "Object property not set"));
}
