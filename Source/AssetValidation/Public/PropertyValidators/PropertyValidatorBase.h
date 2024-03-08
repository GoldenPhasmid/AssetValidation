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
	FFieldClass* GetPropertyClass() const;
	
	/**
	 * @brief Determines whether given property can be validated by this validator
	 * @param Property property to validate
	 * @return can property be validated
	 */
	virtual bool CanValidateProperty(const FProperty* Property) const;

	/**
	 * @brief Determines whether given property 
	 * @param Property property to validate
	 * @param Value property value to validate
	 * @return 
	 */
	virtual bool CanValidatePropertyValue(const FProperty* Property, const void* Value) const;
	
	/**
	 * @brief 
	 * @param Container 
	 * @param Property 
	 * @param ValidationContext 
	 */
	virtual void ValidateProperty(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const
	PURE_VIRTUAL(ValidateProperty)

	/**
	 * @brief validates value property nested in parent property, either struct or container
	 * @param Value pointer to a property value
	 * @param ParentProperty parent property, typically container or struct
	 * @param ValueProperty value property
	 */
	virtual void ValidatePropertyValue(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const
	PURE_VIRTUAL(ValidatePropertyValue)

protected:

	FFieldClass* PropertyClass = nullptr;
};
