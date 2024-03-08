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
	virtual bool CanValidateProperty(const FProperty* Property) const override;
	virtual void ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	virtual void ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
