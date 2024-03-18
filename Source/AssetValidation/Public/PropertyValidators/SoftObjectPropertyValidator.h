#pragma once

#include "PropertyValidatorBase.h"

#include "SoftObjectPropertyValidator.generated.h"

UCLASS()
class USoftObjectPropertyValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:

	USoftObjectPropertyValidator();

	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
