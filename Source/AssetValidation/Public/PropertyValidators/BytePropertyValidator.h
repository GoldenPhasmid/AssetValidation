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
	virtual bool CanValidateProperty(const FProperty* Property) const override;
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const override;
	//~End PropertyValidatorBase interface
};
