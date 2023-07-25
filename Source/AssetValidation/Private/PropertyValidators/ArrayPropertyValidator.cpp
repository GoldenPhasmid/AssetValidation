#include "PropertyValidators/ArrayPropertyValidator.h"

#include "AssetValidationStatics.h"
#include "PropertyValidatorSubsystem.h"

bool UArrayPropertyValidator::CanValidateProperty(FProperty* Property) const
{
	return AssetValidationStatics::CanValidateProperty(Property) && Property->IsA<FArrayProperty>();
}

bool UArrayPropertyValidator::CanValidatePropertyValue(FProperty* ParentProperty, FProperty* ValueProperty) const
{
	return false;
}

void UArrayPropertyValidator::ValidateProperty(UObject* Object, FProperty* Property, FPropertyValidationResult& OutValidationResult) const
{
	UPropertyValidatorSubsystem* PropertyValidators = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	check(PropertyValidators);
	
	FArrayProperty* ArrayProperty = CastFieldChecked<FArrayProperty>(Property);
	FProperty* ValueProperty = ArrayProperty->Inner;
	FScriptArray* Array = ArrayProperty->GetPropertyValuePtr(Property->ContainerPtrToValuePtr<void>(Object));

	const uint32 Num = Array->Num();
	const uint32 Stride = ArrayProperty->Inner->ElementSize;

	uint8* Data = static_cast<uint8*>(Array->GetData());
	for (uint32 Index = 0; Index < Num; ++Index)
	{
		PropertyValidators->IsPropertyValueValid(Data, ArrayProperty, ValueProperty, OutValidationResult);
		Data += Stride;
	}
	
}

void UArrayPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationResult& OutValidationResult) const
{
	// array property value is always valid
}
