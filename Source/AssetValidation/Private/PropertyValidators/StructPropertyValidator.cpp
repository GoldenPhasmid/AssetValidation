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

void UStructPropertyValidator::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FStructProperty* StructProperty = CastFieldChecked<FStructProperty>(Property);

	const uint8* ContainerPtr = StructProperty->ContainerPtrToValuePtr<uint8>(PropertyMemory);
	
	// struct value is validated by UStructValidator objects
	ValidationContext.PushPrefix(Property->GetName());
	// validate underlying struct properties: structure becomes a property container
	ValidationContext.IsPropertyContainerValid(ContainerPtr, StructProperty->Struct);
	ValidationContext.PopPrefix();
}
