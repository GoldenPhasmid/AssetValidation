#include "PropertyValidators/PropertyValidatorBase.h"

#include "AssetValidationStatics.h"
#include "PropertyValidatorSubsystem.h"

FFieldClass* UPropertyValidatorBase::GetPropertyClass() const
{
	return PropertyClass;
}

bool UPropertyValidatorBase::CanValidateProperty(FProperty* Property) const
{
	// don't check for property type as property validator is obtained by underlying property class
	return Property && AssetValidationStatics::CanValidateProperty(Property) && Property->HasMetaData(ValidationNames::Validate);
}
