#include "ObjectContainerValidator.h"

#include "PropertyValidatorSubsystem.h"
#include "Editor/MetaDataContainer.h"
#include "PropertyValidators/PropertyValidation.h"

UObjectContainerValidator::UObjectContainerValidator()
{
	PropertyClass = FObjectPropertyBase::StaticClass();
}

bool UObjectContainerValidator::CanValidateProperty(const FProperty* Property, FMetaDataSource& MetaData) const
{
	// @todo: validate only object and soft object properties
	return Super::CanValidateProperty(Property, MetaData) && MetaData.HasMetaData(UE::AssetValidation::ValidateRecursive);
}

void UObjectContainerValidator::ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
{
	const FObjectPropertyBase* ObjectProperty = CastFieldChecked<FObjectPropertyBase>(Property);

	if (const UObject* Object = ObjectProperty->LoadObjectPropertyValue(PropertyMemory))
	{
		ValidationContext.PushPrefix(Property->GetName() + "." + Object->GetName());
		// validate underlying object recursively
		ValidationContext.IsPropertyContainerValid(reinterpret_cast<const uint8*>(Object), Object->GetClass());
		ValidationContext.PopPrefix();
	}
}
