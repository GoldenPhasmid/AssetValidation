#pragma once

#include "ContainerValidator.h"

#include "ObjectContainerValidator.generated.h"

UCLASS()
class ASSETVALIDATION_API UObjectContainerValidator: public UContainerValidator
{
	GENERATED_BODY()
public:

	UObjectContainerValidator();

	//~Begin ContainerPropertyValidator
	virtual bool CanValidateProperty(const FProperty* Property, FMetaDataSource& MetaData) const override;
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;
	//~End ContainerPropertyValidator
};