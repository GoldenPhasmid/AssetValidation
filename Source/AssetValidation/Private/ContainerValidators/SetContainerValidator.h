#pragma once

#include "PropertyContainerValidator.h"

#include "SetContainerValidator.generated.h"

UCLASS()
class ASSETVALIDATION_API USetContainerValidator: public UPropertyContainerValidator
{
	GENERATED_BODY()
public:
	USetContainerValidator();

	//~Begin ContainerPropertyValidator
	virtual bool CanValidateContainerProperty(const FProperty* Property) const override;
	virtual void ValidateContainerProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	//~End ContainerPropertyValidator
};
