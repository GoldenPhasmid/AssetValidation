#pragma once

#include "PropertyContainerValidator.h"

#include "ArrayContainerValidator.generated.h"

UCLASS()
class ASSETVALIDATION_API UArrayContainerValidator: public UPropertyContainerValidator
{
	GENERATED_BODY()
public:

	UArrayContainerValidator();

	//~Begin ContainerPropertyValidator
	virtual bool CanValidateContainerProperty(const FProperty* Property) const override;
	virtual void ValidateContainerProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	//~End ContainerPropertyValidator
};
