#pragma once

#include "PropertyValidators/PropertyValidatorBase.h"

#include "StringPropertyValidator.generated.h"

UCLASS()
class UStringPropertyValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:

	UStringPropertyValidator();
	
	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
