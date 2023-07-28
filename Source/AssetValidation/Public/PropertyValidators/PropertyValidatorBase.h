#pragma once


#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "PropertyValidatorBase.generated.h"

class FPropertyValidationContext;

/**
 * PropertyValidatorBase verifies that a property value meets non-empty requirements
 * For UObject property type that means that UObject is set.
 *
 * All property validators are gathered using GetDerivedClasses
 */
UCLASS(Abstract)
class ASSETVALIDATION_API UPropertyValidatorBase: public UObject
{
	GENERATED_BODY()
public:

	/** @return property class that this validator operates on */
	virtual FFieldClass* GetPropertyClass() const;
	
	/**
	 * @brief Determines whether given property can be validated by this validator
	 * @param Property property to validate
	 * @return can property be validated
	 */
	virtual bool CanValidateProperty(FProperty* Property) const;
	
	/**
	 * @brief 
	 * @param Property 
	 * @param BasePointer
	 */
	virtual void ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationContext& ValidationContext) const PURE_VIRTUAL(ValidateProperty)

	/**
	 * @brief validates value property nested in parent property, either struct or container
	 * @param Value pointer to a property value
	 * @param ParentProperty parent property, typically container or struct
	 * @param ValueProperty value property
	 */
	virtual void ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const PURE_VIRTUAL(ValidatePropertyValue)

protected:

	FFieldClass* PropertyClass = nullptr;
};
