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
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
