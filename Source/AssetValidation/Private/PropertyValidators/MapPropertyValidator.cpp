#include "PropertyValidators/MapPropertyValidator.h"

#include "PropertyValidatorSubsystem.h"
#include "PropertyValidators/PropertyValidation.h"

UMapPropertyValidator::UMapPropertyValidator()
{
	PropertyClass = FMapProperty::StaticClass();
}

void UMapPropertyValidator::ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationContext& ValidationContext) const
{
	FMapProperty* MapProperty = CastFieldChecked<FMapProperty>(Property);
	FScriptMap* Map = MapProperty->GetPropertyValuePtr(MapProperty->ContainerPtrToValuePtr<void>(BasePointer));

	FProperty* KeyProperty = MapProperty->KeyProp;
	FProperty* ValueProperty = MapProperty->ValueProp;

	const FScriptMapLayout MapLayout = Map->GetScriptLayout(
		KeyProperty->GetSize(), KeyProperty->GetMinAlignment(),
		ValueProperty->GetSize(), ValueProperty->GetMinAlignment()
	);
	const uint32 Num = Map->GetMaxIndex();

	for (uint32 Index = 0; Index < Num; ++Index)
	{
		uint8* Data = static_cast<uint8*>(Map->GetData(Index, MapLayout));

		// validate key property value
		ValidationContext.IsPropertyValueValid(Data, MapProperty, KeyProperty);

		// offset to value property
		Data += MapLayout.ValueOffset;

		// validate value property value
		ValidationContext.IsPropertyValueValid(Data, MapProperty, ValueProperty);
	}

}

void UMapPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
{
	// map property value always valid
}
