#pragma once


#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "PropertyValidatorBase.generated.h"

struct FPropertyValidationResult;

struct FPropertyValidationResult
{
	void Append(const FPropertyValidationResult& Other)
	{
		ValidationErrors.Append(Other.ValidationErrors);
		// either Other or this has validation errors, Validation Result becomes Invalid
		if (ValidationErrors.Num() > 0)
		{
			ValidationResult = EDataValidationResult::Invalid;
		}
		// handle NotValidated;NotValidated - Valid;NotValidated - NotValidated;Valid - Valid;Valid cases
		else if (ValidationResult != Other.ValidationResult)
		{
			ValidationResult = EDataValidationResult::Valid;
		}
	}
	
	void PropertyFails(FProperty* Property, const FText& ErrorText = FText::GetEmpty())
	{
		ValidationResult = EDataValidationResult::Invalid;
		if (!ErrorText.IsEmpty())
		{
			// @todo: construct failure message with @Property
			ValidationErrors.Add(ErrorText);
		}
	}
	
	void PropertyPasses(FProperty* Property)
	{
		if (ValidationResult != EDataValidationResult::Invalid)
		{
			ValidationResult = EDataValidationResult::Valid;
		}
	}
	
	FString Parent;
	TArray<FText> ValidationErrors;
	EDataValidationResult ValidationResult = EDataValidationResult::NotValidated;
};

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
	/**
	 * @brief Determines whether given property can be validated by this validator
	 * @param Property property to validate
	 * @return can property be validated
	 */
	virtual bool CanValidateProperty(FProperty* Property) const PURE_VIRTUAL(CanValidateProperty, return false;)
	
	/**
	 * @brief Determines whether property inside parent can be validated by this validator
	 * @param ParentProperty parent property, either container or a struct
	 * @param ValueProperty property that holds a value to validate
	 * @return can property be validated
	 */
	virtual bool CanValidatePropertyValue(FProperty* ParentProperty, FProperty* ValueProperty) const PURE_VIRTUAL(CanValidatePropertyValue, return false;)
	
	/**
	 * @brief 
	 * @param Property 
	 * @param BasePointer
	 * @param OutValidationResult validation result
	 */
	virtual void ValidateProperty(FProperty* Property, void* BasePointer, FPropertyValidationResult& OutValidationResult) const PURE_VIRTUAL(ValidateProperty)

	/**
	 * @brief validates value property nested in parent property, either struct or container
	 * @param Value pointer to a property value
	 * @param ParentProperty parent property, typically container or struct
	 * @param ValueProperty value property
	 * @param OutValidationResult validation result 
	 */
	virtual void ValidatePropertyValue(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationResult& OutValidationResult) const PURE_VIRTUAL(ValidatePropertyValue)

	
};
