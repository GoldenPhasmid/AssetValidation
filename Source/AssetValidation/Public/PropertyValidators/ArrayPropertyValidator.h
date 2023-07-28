#pragma once

#include "PropertyValidatorBase.h"

#include "ArrayPropertyValidator.generated.h"

UCLASS()
class UArrayPropertyValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:

	UArrayPropertyValidator();
	
	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationContext& ValidationContext) const override;
	virtual void ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
