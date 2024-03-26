#include "MapContainerValidator.h"

#include "PropertyValidationSettings.h"
#include "PropertyValidatorSubsystem.h"
#include "Editor/MetaDataContainer.h"
#include "PropertyValidators/PropertyValidation.h"

UMapContainerValidator::UMapContainerValidator()
{
	PropertyClass = FMapProperty::StaticClass();
}

bool UMapContainerValidator::CanValidateProperty(const FProperty* Property, FMetaDataSource& MetaData) const
{
	if (Super::CanValidateProperty(Property, MetaData))
	{
		const FMapProperty* MapProperty = CastFieldChecked<FMapProperty>(Property);

		if (MetaData.HasMetaData(UE::AssetValidation::Validate) || MetaData.HasMetaData(UE::AssetValidation::ValidateKey) || MetaData.HasMetaData(UE::AssetValidation::ValidateValue))
		{
			// any validation meta data: Validate, ValidateKey, ValidateValue
			return true;
		}
		
		if (UPropertyValidationSettings::Get()->bAutoValidateStructInnerProperties && (MapProperty->KeyProp->IsA<FStructProperty>() || MapProperty->ValueProp->IsA<FStructProperty>()))
		{
			// if either key or value property is a struct and we can validate it without meta
			return true;
		}
	}

	return false;
}

void UMapContainerValidator::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	const FMapProperty* MapProperty = CastFieldChecked<FMapProperty>(Property);
	const FScriptMap* Map = MapProperty->GetPropertyValuePtr(PropertyMemory);

	const FProperty* KeyProperty = MapProperty->KeyProp;
	const FProperty* ValueProperty = MapProperty->ValueProp;

	const FScriptMapLayout MapLayout = Map->GetScriptLayout(
	KeyProperty->GetSize(), KeyProperty->GetMinAlignment(),
	ValueProperty->GetSize(), ValueProperty->GetMinAlignment());

	const bool bValidateStructs = UPropertyValidationSettings::Get()->bAutoValidateStructInnerProperties;
	const bool bHasValidateMeta = MetaData.HasMetaData(UE::AssetValidation::Validate);
	const bool bCanValidateKey = bHasValidateMeta || (bValidateStructs && KeyProperty->IsA<FStructProperty>()) || MetaData.HasMetaData(UE::AssetValidation::ValidateKey);
	const bool bCanValidateValue = bHasValidateMeta || (bValidateStructs && ValueProperty->IsA<FStructProperty>()) || MetaData.HasMetaData(UE::AssetValidation::ValidateValue);

	// UPropertyValidateBase::CanValidatePropertyValue usually checks for Validate meta on ParentProperty to continue with actual validation
	// To work with other metas like ValidateKey and ValidateValue (to validate only map key or only map value),
	// we temporarily add Validate meta specifier, so that checks inside other validators pass
	if (!bHasValidateMeta)
	{
		MetaData.SetMetaData(UE::AssetValidation::Validate, FString{});
	}
	
	const uint32 Num = Map->GetMaxIndex();
	for (uint32 Index = 0; Index < Num; ++Index)
	{
		const uint8* Data = static_cast<const uint8*>(Map->GetData(Index, MapLayout));

		// add map property prefix
		ValidationContext.PushPrefix(Property->GetName() + "[" + FString::FromInt(Index) + "]");

		if (bCanValidateKey)
		{
			// validate key property value
			ValidationContext.IsPropertyValueValid(Data, KeyProperty, MetaData);
		}

		// offset to value property
		Data += MapLayout.ValueOffset;

		if (bCanValidateValue)
		{
			// validate value property value
			ValidationContext.IsPropertyValueValid(Data, ValueProperty, MetaData);
		}

		// pop map property prefix
		ValidationContext.PopPrefix();
	}

	if (!bHasValidateMeta)
	{
		// remove Validate meta if it wasn't on map property in a first place
		MetaData.RemoveMetaData(UE::AssetValidation::Validate);
	}
}
