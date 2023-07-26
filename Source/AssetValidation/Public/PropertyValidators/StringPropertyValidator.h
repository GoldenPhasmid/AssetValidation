#pragma once

#include "PropertyValidators/PropertyValidatorBase.h"

#include "StringPropertyValidator.generated.h"

UCLASS()
class UStringPropertyValidator: public UPropertyValidatorBase
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
