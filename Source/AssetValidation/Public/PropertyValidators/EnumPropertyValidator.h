#pragma once

#include "PropertyValidatorBase.h"

#include "EnumPropertyValidator.generated.h"

UCLASS()
class UEnumPropertyValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:
	//~Begin PropertyValidatorBase
	virtual FFieldClass* GetPropertyClass() const override;
	virtual void ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationContext& ValidationContext) const override;
	virtual void ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
