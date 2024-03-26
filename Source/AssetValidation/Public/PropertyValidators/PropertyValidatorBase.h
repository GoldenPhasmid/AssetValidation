#pragma once


#include "CoreMinimal.h"
#include "Editor/PropertyValidationVariableDetailCustomization.h"
#include "Templates/NonNullPointer.h"
#include "UObject/Object.h"

#include "PropertyValidatorBase.generated.h"

class FPropertyValidationContext;
namespace UE::AssetValidation
{
	class FMetaDataSource;
}
using FMetaDataSource = UE::AssetValidation::FMetaDataSource;

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
	 * @param MetaData meta data source, may not match property metadata
	 * @return can property be validated
	 */
	virtual bool CanValidateProperty(const FProperty* Property, FMetaDataSource& MetaData) const;
	
	/**
	 * @brief validates property value defined by property memory
	 * @param PropertyMemory pointer to a property value
	 * @param Property property that can be analyzed by this property validator
	 * @param MetaData meta data source that should be used when querying meta data from property
	 * @param ValidationContext validation context
	 */
	virtual void ValidateProperty(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, FMetaDataSource& MetaData, FPropertyValidationContext& ValidationContext) const
	PURE_VIRTUAL(ValidateProperty)

protected:

	FFieldClass* PropertyClass = nullptr;
};
