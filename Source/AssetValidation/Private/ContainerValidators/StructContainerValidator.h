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
	virtual bool CanValidateProperty(const FProperty* Property) const override;
	virtual void ValidateProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	//~End ContainerPropertyValidator
};
