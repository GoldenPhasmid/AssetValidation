#pragma once

#include "PropertyValidatorBase.h"

#include "ObjectPropertyValidator.generated.h"

UCLASS()
class UObjectPropertyValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:

	UObjectPropertyValidator();
	
	//~Begin PropertyValidatorBase
	virtual bool CanValidateProperty(FProperty* Property) const override;
	virtual void ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationContext& ValidationContext) const override;
	virtual void ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
