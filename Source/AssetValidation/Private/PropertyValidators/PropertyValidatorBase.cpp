#include "PropertyValidators/PropertyValidatorBase.h"

#include "PropertyValidatorSubsystem.h"
#include "Editor/MetaDataSource.h"
#include "PropertyValidators/PropertyValidation.h"

FFieldClass* UPropertyValidatorBase::GetPropertyClass() const
{
	return PropertyClass;
}

bool UPropertyValidatorBase::CanValidateProperty(const FProperty* Property, FMetaDataSource& MetaData) const
{
	// check for property type
	if (Property && Property->IsA(PropertyClass))
	{
		// check for Validate meta by default
		if (MetaData.HasMetaData(UE::AssetValidation::Validate))
		{
			return true;
		}
	}

	return false;
}
