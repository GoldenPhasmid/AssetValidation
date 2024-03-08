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
	virtual bool CanValidateProperty(const FProperty* Property) const override;
	virtual void ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	virtual void ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
