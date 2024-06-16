#pragma once

#include "ContainerValidator.h"

#include "ArrayContainerValidator.generated.h"

UCLASS()
class ASSETVALIDATION_API UArrayContainerValidator: public UContainerValidator
{
	GENERATED_BODY()
public:

	UArrayContainerValidator();

	//~Begin ContainerPropertyValidator
	virtual bool CanValidateProperty(const FProperty* Property, FMetaDataSource& MetaData) const override;
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;
	//~End ContainerPropertyValidator
};
