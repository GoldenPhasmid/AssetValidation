#pragma once

#include "CoreMinimal.h"
#include "PropertyValidators/PropertyValidatorBase.h"

#include "ContainerValidator.generated.h"

class FPropertyValidationContext;

UCLASS(Abstract)
class ASSETVALIDATION_API UContainerValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:

	virtual bool CanValidateProperty(const FProperty* Property, FMetaDataSource& MetaData) const override;
};
