#include "SetContainerValidator.h"

#include "PropertyValidators/PropertyValidation.h"

extern bool GValidateStructPropertiesWithoutMeta;

USetContainerValidator::USetContainerValidator()
{
	PropertyClass = FSetProperty::StaticClass();
}

bool USetContainerValidator::CanValidateContainerProperty(const FProperty* Property) const
{
	if (Super::CanValidateContainerProperty(Property))
	{
		if (Property->HasMetaData(ValidationNames::Validate))
		{
			// in general, array contents should be validated if Validate meta is present
			return true;
		}

		if (GValidateStructPropertiesWithoutMeta)
		{
			// cast checked because Super call checks for property compatibility
			const FSetProperty* SetProperty = CastFieldChecked<FSetProperty>(Property);
			if (SetProperty->ElementProp->IsA<FStructProperty>())
			{
				// allow struct properties to be recursively validated without meta specifier
				return true;
			}
		}
	}

	return false;
}

void USetContainerValidator::ValidateContainerProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FSetProperty* SetProperty = CastFieldChecked<FSetProperty>(Property);
	const FScriptSet* Set = SetProperty->GetPropertyValuePtr(PropertyMemory);

	const FProperty* ValueProperty = SetProperty->ElementProp;
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
