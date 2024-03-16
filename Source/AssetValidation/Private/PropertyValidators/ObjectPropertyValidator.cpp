#include "PropertyValidators/ObjectPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

UObjectPropertyValidator::UObjectPropertyValidator()
{
	PropertyClass = FObjectProperty::StaticClass();
}

const FText ObjectEmptyFailure = NSLOCTEXT("AssetValidation", "ObjectProperty", "Object property not set");

void UObjectPropertyValidator::ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	UObject* const* ObjectPtr = Property->ContainerPtrToValuePtr<UObject*>(Container);
    check(ObjectPtr);

	UObject* Object = *ObjectPtr;
	const bool bObjectValid = Object != nullptr && Object->IsValidLowLevel();

	ValidationContext.FailOnCondition(!bObjectValid, Property, ObjectEmptyFailure, Property->GetDisplayNameText());
}

void UObjectPropertyValidator::ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	UObject* Object = *static_cast<UObject* const*>(Value);
	
	const bool bObjectValid = Object != nullptr && Object->IsValidLowLevel();
	ValidationContext.FailOnCondition(!bObjectValid, ParentProperty, ObjectEmptyFailure);
}
