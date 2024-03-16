#pragma once

#include "PropertyValidatorBase.h"

#include "ArrayPropertyValidator.generated.h"

UCLASS(Abstract)
class UArrayPropertyValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:

	UArrayPropertyValidator();
	
	//~Begin PropertyValidatorBase
	virtual bool CanValidateProperty(const FProperty* Property) const override;
	virtual void ValidateProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
