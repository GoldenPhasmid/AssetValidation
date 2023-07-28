#pragma once

#include "PropertyValidators/PropertyValidatorBase.h"

#include "StringPropertyValidator.generated.h"

UCLASS()
class UStringPropertyValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:

	UStringPropertyValidator();
	
	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	virtual void ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
