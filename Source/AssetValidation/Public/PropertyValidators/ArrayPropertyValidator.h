#pragma once

#include "PropertyValidatorBase.h"

#include "ArrayPropertyValidator.generated.h"

UCLASS()
class UArrayPropertyValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:
	
	//~Begin PropertyValidatorBase
	virtual bool CanValidateProperty(FProperty* Property) const override;
	virtual bool CanValidatePropertyValue(FProperty* ParentProperty, FProperty* ValueProperty) const override;
	virtual void ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationResult& OutValidationResult) const override;
	virtual void ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationResult& OutValidationResult) const override;
	//~End PropertyValidatorBase
};
