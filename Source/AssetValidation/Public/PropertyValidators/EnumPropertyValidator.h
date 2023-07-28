#pragma once

#include "PropertyValidatorBase.h"

#include "EnumPropertyValidator.generated.h"

UCLASS()
class UEnumPropertyValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:

	UEnumPropertyValidator();
	
	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	virtual void ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
