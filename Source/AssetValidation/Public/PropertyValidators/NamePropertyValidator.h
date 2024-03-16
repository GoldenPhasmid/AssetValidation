#pragma once

#include "PropertyValidators/PropertyValidatorBase.h"

#include "NamePropertyValidator.generated.h"

UCLASS()
class UNamePropertyValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:

	UNamePropertyValidator();
	
	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
