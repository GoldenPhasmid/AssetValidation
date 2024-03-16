#pragma once

#include "PropertyContainerValidator.h"

#include "ObjectContainerValidator.generated.h"

UCLASS()
class ASSETVALIDATION_API UObjectContainerValidator: public UPropertyContainerValidator
{
	GENERATED_BODY()
public:

	UObjectContainerValidator();

	//~Begin ContainerPropertyValidator
	virtual bool CanValidateContainerProperty(const FProperty* Property) const override;
	virtual void ValidateContainerProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	//~End ContainerPropertyValidator
};