#include "ContainerValidators/ContainerValidator.h"

bool UContainerValidator::CanValidateProperty(const FProperty* Property, FMetaDataSource& MetaData) const
{
	// do not require Validate meta in container validation by default
	return Descriptor.Matches(Property);
}
