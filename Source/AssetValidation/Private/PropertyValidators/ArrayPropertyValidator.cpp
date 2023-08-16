#include "PropertyValidators/ArrayPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

UArrayPropertyValidator::UArrayPropertyValidator()
{
	PropertyClass = FArrayProperty::StaticClass();
}

void UArrayPropertyValidator::ValidateProperty(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	FArrayProperty* ArrayProperty = CastFieldChecked<FArrayProperty>(Property);
	FProperty* ValueProperty = ArrayProperty->Inner;
	FScriptArray* Array = ArrayProperty->GetPropertyValuePtr(Property->ContainerPtrToValuePtr<void>(Container));

	const uint32 Num = Array->Num();
	const uint32 Stride = ArrayProperty->Inner->ElementSize;

	uint8* Data = static_cast<uint8*>(Array->GetData());
	for (uint32 Index = 0; Index < Num; ++Index)
	{
		ValidationContext.PushPrefix(Property->GetName() + "[" + FString::FromInt(Index) + "]");
		// validate property value
		ValidationContext.IsPropertyValueValid(Data, ArrayProperty, ValueProperty);
		ValidationContext.PopPrefix();
		
		Data += Stride;
	}
}

void UArrayPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	// array property value is always valid
}
