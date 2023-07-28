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
	virtual bool CanValidateProperty(FProperty* Property) const override;
	virtual void ValidateProperty(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	virtual void ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
