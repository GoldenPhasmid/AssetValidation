#include "ObjectContainerValidator.h"

#include "PropertyValidatorSubsystem.h"
#include "PropertyValidators/PropertyValidation.h"

UObjectContainerValidator::UObjectContainerValidator()
{
	PropertyClass = FObjectPropertyBase::StaticClass();
}

bool UObjectContainerValidator::CanValidateContainerProperty(const FProperty* Property) const
{
	return Super::CanValidateContainerProperty(Property) && Property->HasMetaData(ValidationNames::ValidateRecursive);
}

void UObjectContainerValidator::ValidateContainerProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
{
	const FObjectPropertyBase* ObjectProperty = CastFieldChecked<FObjectPropertyBase>(Property);

	if (const UObject* Object = ObjectProperty->LoadObjectPropertyValue(PropertyMemory))
	{
		ValidationContext.PushPrefix(Property->GetName() + "." + Object->GetName());
		// validate underlying object recursively
		ValidationContext.IsPropertyContainerValid(Object, Object->GetClass());
		ValidationContext.PopPrefix();
	}
}
