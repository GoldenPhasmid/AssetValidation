#pragma once

#include "PropertyContainerValidator.h"

#include "StructContainerValidator.generated.h"

/**
 * Base class for struct containers. Generic implementation explicitly treats struct as a container of properties
 * Other implementations may call 
 */
UCLASS()
class UStructContainerValidator: public UPropertyContainerValidator
{
	GENERATED_BODY()
public:

	UStructContainerValidator();

	//~Begin ContainerPropertyValidator
	virtual bool CanValidateProperty(const FProperty* Property, FMetaDataSource& MetaData) const override;
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const override;
	//~End ContainerPropertyValidator

	virtual void ValidateStructAsContainer(TNonNullPtr<const uint8> PropertyMemory, const FStructProperty* StructProperty, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const;
};
