#include "PropertyContainerValidator.h"

bool UPropertyContainerValidator::CanValidateProperty(const FProperty* Property) const
{
	// do not require Validate meta in container validation by default
	return Property && Property->IsA(PropertyClass);
}
