#include "PropertyValidators/SetPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

USetPropertyValidator::USetPropertyValidator()
{
	PropertyClass = FSetProperty::StaticClass();
}

bool USetPropertyValidator::CanValidateProperty(const FProperty* Property) const
{
	if (const FSetProperty* SetProperty = CastField<FSetProperty>(Property))
	{
		if (FStructProperty* ValueProperty = CastField<FStructProperty>(SetProperty->ElementProp))
		{
			// allow struct properties to be recursively validated without meta specifier
			return true;
		}

		// otherwise, set property should have Validate meta to validate its contents
		return SetProperty->HasMetaData(ValidationNames::Validate);
	}
	
	return false;
}

void USetPropertyValidator::ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FSetProperty* SetProperty = CastFieldChecked<FSetProperty>(Property);
	FProperty* ValueProperty = SetProperty->ElementProp;
	const FScriptSet* Set = SetProperty->GetPropertyValuePtr(Property->ContainerPtrToValuePtr<void>(Container));

	const FScriptSetLayout Layout = Set->GetScriptLayout(ValueProperty->GetSize(), ValueProperty->GetMinAlignment());
	
	const uint32 Num = Set->Num();
	for (uint32 Index = 0; Index < Num; ++Index)
	{
		const void* Data = Set->GetData(Index, Layout);

		ValidationContext.PushPrefix(Property->GetName() + "[" + FString::FromInt(Index) + "]");
		// validate property value
		ValidationContext.IsPropertyValueValid(Data, SetProperty, ValueProperty);
		ValidationContext.PopPrefix();
	}
}

void USetPropertyValidator::ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	checkNoEntry();
	// set property value is always valid
}
