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
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
