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
	virtual void ValidateProperty(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	virtual void ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
