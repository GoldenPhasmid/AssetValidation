#include "PropertyValidators/MapPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

UMapPropertyValidator::UMapPropertyValidator()
{
	PropertyClass = FMapProperty::StaticClass();
}

bool UMapPropertyValidator::CanValidateProperty(FProperty* Property) const
{
	return Super::CanValidateProperty(Property) || Property->HasMetaData(ValidationNames::ValidateKey) || Property->HasMetaData(ValidationNames::ValidateValue);
}

void UMapPropertyValidator::ValidateProperty(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	FMapProperty* MapProperty = CastFieldChecked<FMapProperty>(Property);
	FScriptMap* Map = MapProperty->GetPropertyValuePtr(MapProperty->ContainerPtrToValuePtr<void>(Container));

	FProperty* KeyProperty = MapProperty->KeyProp;
	FProperty* ValueProperty = MapProperty->ValueProp;

	const FScriptMapLayout MapLayout = Map->GetScriptLayout(
		KeyProperty->GetSize(), KeyProperty->GetMinAlignment(),
		ValueProperty->GetSize(), ValueProperty->GetMinAlignment()
	);
	const bool bCanValidate = Property->HasMetaData(ValidationNames::Validate);
	
	const uint32 Num = Map->GetMaxIndex();
	for (uint32 Index = 0; Index < Num; ++Index)
	{
		uint8* Data = static_cast<uint8*>(Map->GetData(Index, MapLayout));

		// add map property prefix
		ValidationContext.PushPrefix(Property->GetName() + "[" + FString::FromInt(Index) + "]");

		if (bCanValidate || Property->HasMetaData(ValidationNames::ValidateKey))
		{
			// validate key property value
			ValidationContext.IsPropertyValueValid(Data, MapProperty, KeyProperty);
		}

		// offset to value property
		Data += MapLayout.ValueOffset;

		if (bCanValidate || Property->HasMetaData(ValidationNames::ValidateValue))
		{
			// validate value property value
			ValidationContext.IsPropertyValueValid(Data, MapProperty, ValueProperty);
		}

		// pop map property prefix
		ValidationContext.PopPrefix();
	}

}

void UMapPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	// map property value always valid
}
