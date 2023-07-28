#include "PropertyValidators/StructPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

UStructPropertyValidator::UStructPropertyValidator()
{
	PropertyClass = FStructProperty::StaticClass();
}

void UStructPropertyValidator::ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationContext& ValidationContext) const
{
	const FStructProperty* StructProperty = CastFieldChecked<FStructProperty>(Property);

	void* ContainerPtr = StructProperty->ContainerPtrToValuePtr<void*>(BasePointer);
	//@todo: validate struct value?
	// validate underlying struct properties: structure becomes a property container
	ValidationContext.IsPropertyContainerValid(ContainerPtr, StructProperty->Struct);
}

void UStructPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	ValidationContext.IsPropertyValueValid(Value, ParentProperty, ValueProperty);
}
