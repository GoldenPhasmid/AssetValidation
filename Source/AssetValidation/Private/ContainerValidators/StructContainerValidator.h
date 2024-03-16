#pragma once

#include "PropertyContainerValidator.h"

#include "StructContainerValidator.generated.h"

UCLASS()
class UStructContainerValidator: public UPropertyContainerValidator
{
	GENERATED_BODY()
public:

	UStructContainerValidator();

	//~Begin ContainerPropertyValidator
	virtual bool CanValidateContainerProperty(const FProperty* Property) const override;
	virtual void ValidateContainerProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	//~End ContainerPropertyValidator
};
