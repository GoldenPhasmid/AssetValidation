#pragma once

#include "CoreMinimal.h"
#include "PropertyValidators/PropertyValidatorBase.h"

#include "PropertyContainerValidator.generated.h"

class FPropertyValidationContext;

UCLASS(Abstract)
class ASSETVALIDATION_API UPropertyContainerValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:

	virtual bool CanValidateProperty(const FProperty* Property) const override;
};
