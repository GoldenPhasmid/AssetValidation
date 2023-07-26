#include "PropertyValidators/MapPropertyValidator.h"

#include "AssetValidationStatics.h"
#include "PropertyValidatorSubsystem.h"

bool UMapPropertyValidator::CanValidateProperty(FProperty* Property) const
{
	return AssetValidationStatics::CanValidateProperty(Property) && Property->IsA<FMapProperty>();
}

bool UMapPropertyValidator::CanValidatePropertyValue(FProperty* ParentProperty, FProperty* ValueProperty) const
{
	return false;
}

void UMapPropertyValidator::ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationResult& OutValidationResult) const
{
	UPropertyValidatorSubsystem* PropertyValidators = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	check(PropertyValidators);
	
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
		PropertyValidators->IsPropertyValueValid(Data, MapProperty, KeyProperty, OutValidationResult);

		// offset to value property
		Data += MapLayout.ValueOffset;

		// validate value property value
		PropertyValidators->IsPropertyValueValid(Data, MapProperty, ValueProperty, OutValidationResult);
	}

}

void UMapPropertyValidator::ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationResult& OutValidationResult) const
{
	// map property value always valid
}
