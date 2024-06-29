#include "PropertyValidators/PropertyValidatorBase.h"

#include "Editor/MetaDataSource.h"
#include "PropertyValidators/PropertyValidation.h"

bool FPropertyValidatorDescriptor::Matches(const FProperty* Property) const
{
	return Property && Property->IsA(PropertyClass) && (CppType == NAME_None || CppType == Property->GetCPPType());
}

bool UPropertyValidatorBase::CanValidateProperty(const FProperty* Property, FMetaDataSource& MetaData) const
{
	// check if validator descriptor matches the property
	if (Descriptor.Matches(Property))
	{
		// check for Validate meta by default
		if (MetaData.HasMetaData(UE::AssetValidation::Validate))
		{
			return true;
		}
	}

	return false;
}
