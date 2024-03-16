#include "StructContainerValidator.h"

#include "PropertyValidatorSubsystem.h"
#include "PropertyValidators/PropertyValidation.h"

extern bool GValidateStructPropertiesWithoutMeta;

UStructContainerValidator::UStructContainerValidator()
{
	PropertyClass = FStructProperty::StaticClass();
}

bool UStructContainerValidator::CanValidateContainerProperty(const FProperty* Property) const
{
	if (Super::CanValidateContainerProperty(Property))
	{
		// do not require meta = (Validate) to perform validation for struct properties.
		// Use Validate meta for structs when you want to validate struct "value", not the underlying struct properties
		return GValidateStructPropertiesWithoutMeta || Property->HasMetaData(ValidationNames::Validate);
	}

	return false;
}

void UStructContainerValidator::ValidateContainerProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FStructProperty* StructProperty = CastFieldChecked<FStructProperty>(Property);

	ValidationContext.PushPrefix(StructProperty->GetName());
	// validate underlying struct properties: structure becomes a property container
	ValidationContext.IsPropertyContainerValid(PropertyMemory, StructProperty->Struct);
	ValidationContext.PopPrefix();
}
