#pragma once

#include "CoreMinimal.h"
#include "PropertyValidatorBase.h"

#include "BytePropertyValidator.generated.h"

UCLASS()
class UBytePropertyValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:
	UBytePropertyValidator();

	//~Begin PropertyValidatorBase interface
	virtual bool CanValidateProperty(const FProperty* Property, FMetaDataSource& MetaData) const override;
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase interface
};
