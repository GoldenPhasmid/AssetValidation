#pragma once

#include "PropertyContainerValidator.h"

#include "SetContainerValidator.generated.h"

UCLASS()
class ASSETVALIDATION_API USetContainerValidator: public UPropertyContainerValidator
{
	GENERATED_BODY()
public:
	USetContainerValidator();

	//~Begin ContainerPropertyValidator
	virtual bool CanValidateProperty(const FProperty* Property, FMetaDataSource& MetaData) const override;
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;
	//~End ContainerPropertyValidator
};
