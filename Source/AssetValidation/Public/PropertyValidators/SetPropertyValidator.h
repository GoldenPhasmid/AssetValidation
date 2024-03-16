#pragma once

#include "PropertyValidatorBase.h"

#include "SetPropertyValidator.generated.h"

UCLASS(Abstract)
class USetPropertyValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:
	
	USetPropertyValidator();
	
	//~Begin PropertyValidatorBase
	virtual bool CanValidateProperty(const FProperty* Property) const override;
	virtual void ValidateProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
