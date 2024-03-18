#pragma once

#include "PropertyValidatorBase.h"

#include "StructPropertyValidator.generated.h"

UCLASS(Abstract)
class UStructPropertyValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:

	UStructPropertyValidator();

	//~Begin PropertyValidatorBase
	virtual bool CanValidateProperty(const FProperty* Property) const override;
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
