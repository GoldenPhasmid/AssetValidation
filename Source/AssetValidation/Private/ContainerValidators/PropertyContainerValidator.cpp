#include "PropertyContainerValidator.h"

FFieldClass* UPropertyContainerValidator::GetContainerPropertyClass() const
{
	return PropertyClass;
}

bool UPropertyContainerValidator::CanValidateContainerProperty(const FProperty* Property) const
{
	return Property && Property->IsA(PropertyClass);
}

