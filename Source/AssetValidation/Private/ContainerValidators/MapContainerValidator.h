#pragma once


#include "PropertyContainerValidator.h"

#include "MapContainerValidator.generated.h"

UCLASS()
class ASSETVALIDATION_API UMapContainerValidator: public UPropertyContainerValidator
{
	GENERATED_BODY()
public:
	UMapContainerValidator();

	//~Begin ContainerPropertyValidator
	virtual bool CanValidateProperty(const FProperty* Property) const override;
	virtual void ValidateProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	//~End ContainerPropertyValidator
};
