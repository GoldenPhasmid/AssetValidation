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
	virtual void ValidateProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
