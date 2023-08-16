#include "PropertyValidators/PropertyValidatorBase.h"

#include "PropertyValidatorSubsystem.h"

FFieldClass* UPropertyValidatorBase::GetPropertyClass() const
{
	return PropertyClass;
}

bool UPropertyValidatorBase::CanValidateProperty(FProperty* Property) const
{
	// don't check for property type as property validator is obtained by underlying property class
	return Property && Property->IsA(PropertyClass) && Property->HasMetaData(ValidationNames::Validate);
}

bool UPropertyValidatorBase::CanValidatePropertyValue(FProperty* Property, void* Value) const
{
	// do not require Validate meta, as it is likely not a 'user defined' property
	return Property && Property->IsA(PropertyClass);
}
