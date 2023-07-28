#include "PropertyValidators/StructPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

UStructPropertyValidator::UStructPropertyValidator()
{
	PropertyClass = FStructProperty::StaticClass();
}

void UStructPropertyValidator::ValidateProperty(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FStructProperty* StructProperty = CastFieldChecked<FStructProperty>(Property);

	void* ContainerPtr = StructProperty->ContainerPtrToValuePtr<void*>(Container);
	//@todo: validate struct value?

	ValidationContext.PushPrefix(Property->GetName());
	// validate underlying struct properties: structure becomes a property container
	ValidationContext.IsPropertyContainerValid(ContainerPtr, StructProperty->Struct);
	ValidationContext.PopPrefix();
}

void UStructPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	// @todo: validate struct value
}
