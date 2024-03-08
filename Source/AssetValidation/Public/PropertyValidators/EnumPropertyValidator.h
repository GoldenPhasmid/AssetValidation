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
	virtual void ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	virtual void ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
