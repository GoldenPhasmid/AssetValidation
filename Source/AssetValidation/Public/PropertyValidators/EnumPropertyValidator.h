#pragma once

#include "PropertyValidatorBase.h"

#include "EnumPropertyValidator.generated.h"

UCLASS()
class UEnumPropertyValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:

	UEnumPropertyValidator();
	
	//~Begin PropertyValidatorBase
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase
};
