#pragma once

#include "PropertyValidatorBase.h"

#include "ObjectPropertyValidator.generated.h"

UCLASS()
class UObjectPropertyValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:

	UObjectPropertyValidator();
	
	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
