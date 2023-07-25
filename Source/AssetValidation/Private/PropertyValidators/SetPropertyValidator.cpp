#include "PropertyValidators/SetPropertyValidator.h"

#include "AssetValidationStatics.h"
#include "PropertyValidatorSubsystem.h"

bool USetPropertyValidator::CanValidateProperty(FProperty* Property) const
{
	return AssetValidationStatics::CanValidateProperty(Property) && Property->IsA<FSetProperty>();
}

bool USetPropertyValidator::CanValidatePropertyValue(FProperty* ParentProperty, FProperty* ValueProperty) const
{
	return false;
}

void USetPropertyValidator::ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationResult& OutValidationResult) const
{
	UPropertyValidatorSubsystem* PropertyValidators = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	check(PropertyValidators);
	
	FSetProperty* SetProperty = CastFieldChecked<FSetProperty>(Property);
	FProperty* ValueProperty = SetProperty->ElementProp;
	FScriptSet* Set = SetProperty->GetPropertyValuePtr(Property->ContainerPtrToValuePtr<void>(BasePointer));

	FScriptSetLayout Layout = Set->GetScriptLayout(ValueProperty->GetSize(), ValueProperty->GetMinAlignment());
	
	const uint32 Num = Set->Num();
	for (uint32 Index = 0; Index < Num; ++Index)
	{
		void* Data = Set->GetData(Index, Layout);

		PropertyValidators->IsPropertyValueValid(Data, SetProperty, ValueProperty, OutValidationResult);
	}
}

void USetPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationResult& OutValidationResult) const
{
	// set property value is always valid
}
