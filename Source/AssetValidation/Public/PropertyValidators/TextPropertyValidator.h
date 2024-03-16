#pragma once

#include "PropertyValidatorBase.h"

#include "TextPropertyValidator.generated.h"

UCLASS()
class UTextPropertyValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:

	UTextPropertyValidator();
	
	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
