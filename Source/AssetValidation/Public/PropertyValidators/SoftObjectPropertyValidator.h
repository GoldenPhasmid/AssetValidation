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
	virtual void ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationContext& ValidationContext) const override;
	virtual void ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
	
};
