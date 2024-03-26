#pragma once


#include "PropertyContainerValidator.h"

#include "MapContainerValidator.generated.h"

UCLASS()
class ASSETVALIDATION_API UMapContainerValidator: public UPropertyContainerValidator
{
	GENERATED_BODY()
public:
	UMapContainerValidator();

	//~Begin ContainerPropertyValidator
	virtual bool CanValidateProperty(const FProperty* Property, FMetaDataSource& MetaData) const override;
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;
	//~End ContainerPropertyValidator
};
