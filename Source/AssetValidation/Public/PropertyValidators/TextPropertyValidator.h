#pragma once

#include "PropertyValidatorBase.h"

#include "TextPropertyValidator.generated.h"

UCLASS()
class UTextPropertyValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:

	UTextPropertyValidator();
	
	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationContext& ValidationContext) const override;
	virtual void ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};