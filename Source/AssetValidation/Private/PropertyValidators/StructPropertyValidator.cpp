#include "PropertyValidators/StructPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

UStructPropertyValidator::UStructPropertyValidator()
{
	PropertyClass = FStructProperty::StaticClass();
}

bool UStructPropertyValidator::CanValidateProperty(const FProperty* Property) const
{
	// do not require meta = (Validate) to perform validation for struct properties.
	// Use Validate meta for structs when you want to validate struct "value", not the underlying struct properties
	return Property && Property->IsA(PropertyClass);
}

void UStructPropertyValidator::ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FStructProperty* StructProperty = CastFieldChecked<FStructProperty>(Property);

	const void* ContainerPtr = StructProperty->ContainerPtrToValuePtr<void*>(Container);
	
	// struct value is validated by UStructValidator objects
	ValidationContext.PushPrefix(Property->GetName());
	// validate underlying struct properties: structure becomes a property container
	ValidationContext.IsPropertyContainerValid(ContainerPtr, StructProperty->Struct);
	ValidationContext.PopPrefix();
}

void UStructPropertyValidator::ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	const FStructProperty* StructProperty = CastFieldChecked<FStructProperty>(ValueProperty);
	
	// struct value is validated by UStructValidator objects

	ValidationContext.PushPrefix(ValueProperty->GetName());
	// validate underlying struct properties: structure becomes a property container
	ValidationContext.IsPropertyContainerValid(Value, StructProperty->Struct);
	ValidationContext.PopPrefix();
}
