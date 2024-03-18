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
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
