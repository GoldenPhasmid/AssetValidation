#pragma once

#include "PropertyValidatorBase.h"

#include "StructPropertyValidator.generated.h"

UCLASS()
class UStructPropertyValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:

	UStructPropertyValidator();

	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	virtual void ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
	
};
