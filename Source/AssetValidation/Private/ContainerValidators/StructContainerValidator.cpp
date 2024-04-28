#include "StructContainerValidator.h"

#include "PropertyValidationSettings.h"
#include "PropertyValidatorSubsystem.h"
#include "Editor/MetaDataSource.h"
#include "PropertyValidators/PropertyValidation.h"

UStructContainerValidator::UStructContainerValidator()
{
	PropertyClass = FStructProperty::StaticClass();
}

bool UStructContainerValidator::CanValidateProperty(const FProperty* Property, FMetaDataSource& MetaData) const
{
	if (Super::CanValidateProperty(Property, MetaData))
	{
		// do not require meta = (Validate) to perform validation for struct properties.
		// Use Validate meta for structs when you want to validate struct "value", not the underlying struct properties
		return UPropertyValidationSettings::Get()->bAutoValidateStructInnerProperties || MetaData.HasMetaData(UE::AssetValidation::Validate);
	}

	return false;
}

void UStructContainerValidator::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	const FStructProperty* StructProperty = CastFieldChecked<FStructProperty>(Property);
	
	// validate underlying struct properties: structure becomes a property container
	const bool bContainerProperty = UE::AssetValidation::IsContainerProperty(StructProperty->GetOwner<FProperty>());
	if (bContainerProperty == false)
	{
		// push property prefix only if owner property is not a container property
		// handles 'struct inside array' type of cases
		ValidationContext.PushPrefix(StructProperty->GetName());
	}
	ValidationContext.IsPropertyContainerValid(PropertyMemory, StructProperty->Struct);
	if (bContainerProperty == false)
	{
		ValidationContext.PopPrefix();
	}
}
