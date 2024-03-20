#include "PropertyValidators/MapPropertyValidator.h"

#include "PropertyValidators/PropertyValidation.h"

UMapPropertyValidator::UMapPropertyValidator()
{
	PropertyClass = FMapProperty::StaticClass();
}

bool UMapPropertyValidator::CanValidateProperty(const FProperty* Property) const
{
	if (const FMapProperty* MapProperty = CastField<FMapProperty>(Property))
	{
		if (MapProperty->IsA(FStructProperty::StaticClass()) || MapProperty->ValueProp->IsA(FStructProperty::StaticClass()))
		{
			// if either key or value property is a struct then we can validate it
			return true;
		}

		// any validation meta data: Validate, ValidateKey, ValidateValue
		return Property->HasMetaData(UE::AssetValidation::Validate) || Property->HasMetaData(UE::AssetValidation::ValidateKey) || Property->HasMetaData(UE::AssetValidation::ValidateValue);
	}
	
	return false;
}

void UMapPropertyValidator::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FMapProperty* MapProperty = CastFieldChecked<FMapProperty>(Property);
	const FScriptMap* Map = MapProperty->GetPropertyValuePtr(MapProperty->ContainerPtrToValuePtr<void>(PropertyMemory));

	FProperty* KeyProperty = MapProperty->KeyProp;
	FProperty* ValueProperty = MapProperty->ValueProp;

	const FScriptMapLayout MapLayout = Map->GetScriptLayout(
		KeyProperty->GetSize(), KeyProperty->GetMinAlignment(),
		ValueProperty->GetSize(), ValueProperty->GetMinAlignment()
	);

	const bool bHasValidateMeta = MapProperty->HasMetaData(UE::AssetValidation::Validate);
	const bool bCanValidateKey = bHasValidateMeta || KeyProperty->IsA(FStructProperty::StaticClass()) || MapProperty->HasMetaData(UE::AssetValidation::ValidateKey);
	const bool bCanValidateValue = bHasValidateMeta || ValueProperty->IsA(FStructProperty::StaticClass()) || MapProperty->HasMetaData(UE::AssetValidation::ValidateValue);

	// UPropertyValidateBase::CanValidatePropertyValue usually checks for Validate meta on ParentProperty to continue with actual validation
	// To work with other metas like ValidateKey and ValidateValue (to validate only map key or only map value),
	// EVERY property validator should have checked the parent property (that it is a map)
	// as well as that it is correct validation meta (not ValidateKey for value property)
	// To simplify our lives, we temporarily set Validate meta, so that checks inside other validators pass
	// while we're checking whether we should validate properties in below for-loop
	const_cast<FMapProperty*>(MapProperty)->SetMetaData(UE::AssetValidation::Validate, FString{});
	
	const uint32 Num = Map->GetMaxIndex();
	for (uint32 Index = 0; Index < Num; ++Index)
	{
		const uint8* Data = static_cast<const uint8*>(Map->GetData(Index, MapLayout));

		// add map property prefix
		ValidationContext.PushPrefix(Property->GetName() + "[" + FString::FromInt(Index) + "]");

		if (bCanValidateKey)
		{
			// validate key property value
			ValidationContext.IsPropertyValueValid(Data, MapProperty, KeyProperty);
		}

		// offset to value property
		Data += MapLayout.ValueOffset;

		if (bCanValidateValue)
		{
			// validate value property value
			ValidationContext.IsPropertyValueValid(Data, MapProperty, ValueProperty);
		}

		// pop map property prefix
		ValidationContext.PopPrefix();
	}

	if (!bHasValidateMeta)
	{
		// remove Validate meta if it wasn't on map property in a first place
		const_cast<FMapProperty*>(MapProperty)->RemoveMetaData(UE::AssetValidation::Validate);
	}
}
