#pragma once

#include "CoreMinimal.h"
#include "Templates/NonNullPointer.h"

#include "PropertyContainerValidator.generated.h"

class FPropertyValidationContext;

UCLASS(Abstract)
class ASSETVALIDATION_API UPropertyContainerValidator: public UObject
{
	GENERATED_BODY()
public:

	/** @return property class that this validator operates on */
	FFieldClass* GetContainerPropertyClass() const;

	/**
	 * @brief Determines whether given container property can be validated by this validator
	 * @param Property property to validate
	 * @return can property be validated
	 */
	virtual bool CanValidateContainerProperty(const FProperty* Property) const;
	
	/**
     * @brief 
     * @param PropertyMemory 
     * @param Property 
     * @param ValidationContext 
     */
    virtual void ValidateContainerProperty(const void* PropertyMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
    PURE_VIRTUAL(ValidateContainerProperty, );

protected:
	
	FFieldClass* PropertyClass = nullptr;
};
