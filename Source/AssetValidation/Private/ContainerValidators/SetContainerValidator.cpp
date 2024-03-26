#include "SetContainerValidator.h"

#include "PropertyValidationSettings.h"
#include "PropertyValidatorSubsystem.h"
#include "Editor/MetaDataContainer.h"
#include "PropertyValidators/PropertyValidation.h"

USetContainerValidator::USetContainerValidator()
{
	PropertyClass = FSetProperty::StaticClass();
}

bool USetContainerValidator::CanValidateProperty(const FProperty* Property, FMetaDataSource& MetaData) const
{
	if (Super::CanValidateProperty(Property, MetaData))
	{
		// cast checked because Super call checks for property compatibility
		const FSetProperty* SetProperty = CastFieldChecked<FSetProperty>(Property);
		const FProperty* InnerProperty = SetProperty->ElementProp;
		
		if (UPropertyValidationSettings::Get()->bAutoValidateStructInnerProperties && InnerProperty->IsA<FStructProperty>())
		{
			// allow struct properties to be recursively validated without meta specifier
			return true;
		}

		const UPropertyValidatorSubsystem* ValidatorSubsystem = UPropertyValidatorSubsystem::Get();
		if (MetaData.HasMetaData(UE::AssetValidation::Validate) && ValidatorSubsystem->HasValidatorForPropertyType(InnerProperty))
		{
			// in general, set contents should be validated if Validate meta is present
			return true;
		}
	}

	return false;
}

void USetContainerValidator::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	const FSetProperty* SetProperty = CastFieldChecked<FSetProperty>(Property);
	const FScriptSet* Set = SetProperty->GetPropertyValuePtr(PropertyMemory);

	const FProperty* ValueProperty = SetProperty->ElementProp;
	const FScriptSetLayout Layout = Set->GetScriptLayout(ValueProperty->GetSize(), ValueProperty->GetMinAlignment());
	
	const uint32 Num = Set->Num();
	for (uint32 Index = 0; Index < Num; ++Index)
	{
		const uint8* Data = static_cast<const uint8*>(Set->GetData(Index, Layout));

		ValidationContext.PushPrefix(Property->GetName() + "[" + FString::FromInt(Index) + "]");
		// validate property value
		ValidationContext.IsPropertyValueValid(Data, ValueProperty, MetaData);
		ValidationContext.PopPrefix();
	}
}
