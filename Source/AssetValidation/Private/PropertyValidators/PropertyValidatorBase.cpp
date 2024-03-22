#include "PropertyValidators/PropertyValidatorBase.h"

#include "PropertyValidatorSubsystem.h"
#include "PropertyValidators/PropertyValidation.h"

FFieldClass* UPropertyValidatorBase::GetPropertyClass() const
{
	return PropertyClass;
}

bool UPropertyValidatorBase::CanValidateProperty(const FProperty* Property) const
{
	// don't check for property type as property validator is obtained by underlying property class
	if (Property && Property->IsA(PropertyClass))
	{
		if (Property->HasMetaData(UE::AssetValidation::Validate))
		{
			return true;
		}

		if (const FProperty* OwnerProperty = Property->GetOwner<FProperty>();
			OwnerProperty && UE::AssetValidation::IsContainerProperty(OwnerProperty) && OwnerProperty->HasMetaData(UE::AssetValidation::Validate))
		{
			return true;
		}
	}

	return false;
}
