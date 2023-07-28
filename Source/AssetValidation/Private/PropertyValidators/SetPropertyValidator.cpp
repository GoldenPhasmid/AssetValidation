#include "PropertyValidators/SetPropertyValidator.h"

#include "PropertyValidatorSubsystem.h"
#include "PropertyValidators/PropertyValidation.h"

USetPropertyValidator::USetPropertyValidator()
{
	PropertyClass = FSetProperty::StaticClass();
}

void USetPropertyValidator::ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationContext& ValidationContext) const
{
	FSetProperty* SetProperty = CastFieldChecked<FSetProperty>(Property);
	FProperty* ValueProperty = SetProperty->ElementProp;
	FScriptSet* Set = SetProperty->GetPropertyValuePtr(Property->ContainerPtrToValuePtr<void>(BasePointer));

	const FScriptSetLayout Layout = Set->GetScriptLayout(ValueProperty->GetSize(), ValueProperty->GetMinAlignment());
	
	const uint32 Num = Set->Num();
	for (uint32 Index = 0; Index < Num; ++Index)
	{
		void* Data = Set->GetData(Index, Layout);

		ValidationContext.PushPrefix(Property->GetName() + "[" + FString::FromInt(Index) + "]");
		// validate property value
		ValidationContext.IsPropertyValueValid(Data, SetProperty, ValueProperty);
		ValidationContext.PopPrefix();
	}
}

void USetPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	// set property value is always valid
}
