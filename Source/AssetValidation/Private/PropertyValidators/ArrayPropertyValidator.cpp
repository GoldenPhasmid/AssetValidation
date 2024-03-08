#include "PropertyValidators/ArrayPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

UArrayPropertyValidator::UArrayPropertyValidator()
{
	PropertyClass = FArrayProperty::StaticClass();
}

bool UArrayPropertyValidator::CanValidateProperty(const FProperty* Property) const
{
	if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
	{
		if (FStructProperty* ValueProperty = CastField<FStructProperty>(ArrayProperty->Inner))
		{
			// allow struct properties to be recursively validated without meta specifier
			return true;
		}

		// otherwise, array property should have Validate meta to validate its contents
		return ArrayProperty->HasMetaData(ValidationNames::Validate);
	}
	
	return false;
}

void UArrayPropertyValidator::ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FArrayProperty* ArrayProperty = CastFieldChecked<FArrayProperty>(Property);
	FProperty* ValueProperty = ArrayProperty->Inner;
	const FScriptArray* Array = ArrayProperty->GetPropertyValuePtr(Property->ContainerPtrToValuePtr<void>(Container));

	const uint32 Num = Array->Num();
	const uint32 Stride = ArrayProperty->Inner->ElementSize;

	const uint8* Data = static_cast<const uint8*>(Array->GetData());
	for (uint32 Index = 0; Index < Num; ++Index)
	{
		ValidationContext.PushPrefix(Property->GetName() + "[" + FString::FromInt(Index) + "]");
		// validate property value
		ValidationContext.IsPropertyValueValid(Data, ArrayProperty, ValueProperty);
		ValidationContext.PopPrefix();
		
		Data += Stride;
	}
}

void UArrayPropertyValidator::ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	checkNoEntry();
	// array property value is always valid
}
