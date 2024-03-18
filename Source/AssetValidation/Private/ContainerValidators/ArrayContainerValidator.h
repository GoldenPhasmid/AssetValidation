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
	virtual bool CanValidateProperty(const FProperty* Property) const override;
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	//~End ContainerPropertyValidator
};
