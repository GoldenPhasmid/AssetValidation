#pragma once

#include "PropertyValidatorBase.h"

#include "SetPropertyValidator.generated.h"

UCLASS()
class USetPropertyValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:
	
	USetPropertyValidator();
	
	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationContext& ValidationContext) const override;
	virtual void ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
