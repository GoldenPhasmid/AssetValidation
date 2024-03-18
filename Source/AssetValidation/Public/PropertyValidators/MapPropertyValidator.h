#pragma once

#include "PropertyValidatorBase.h"

#include "MapPropertyValidator.generated.h"

UCLASS(Abstract)
class UMapPropertyValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:
	
	UMapPropertyValidator();
	
	//~Begin PropertyValidatorBase
	virtual bool CanValidateProperty(const FProperty* Property) const override;
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
