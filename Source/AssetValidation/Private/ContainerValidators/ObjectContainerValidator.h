#pragma once

#include "PropertyContainerValidator.h"

#include "ObjectContainerValidator.generated.h"

UCLASS()
class ASSETVALIDATION_API UObjectContainerValidator: public UPropertyContainerValidator
{
	GENERATED_BODY()
public:

	UObjectContainerValidator();

	//~Begin ContainerPropertyValidator
	virtual bool CanValidateProperty(const FProperty* Property, FMetaDataSource& MetaData) const override;
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;
	//~End ContainerPropertyValidator

	FString FindObjectDisplayName(const UObject* Object) const;
};