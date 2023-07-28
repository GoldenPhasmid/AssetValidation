#pragma once

#include "PropertyValidatorBase.h"

#include "MapPropertyValidator.generated.h"

UCLASS()
class UMapPropertyValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:
	
	UMapPropertyValidator();
	
	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationContext& ValidationContext) const override;
	virtual void ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
