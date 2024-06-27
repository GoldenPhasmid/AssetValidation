#pragma once

#include "CoreMinimal.h"
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
 * Property validator descriptor
 */
USTRUCT()
struct ASSETVALIDATION_API FPropertyValidatorDescriptor
{
	GENERATED_BODY()

	FPropertyValidatorDescriptor() = default;
	FPropertyValidatorDescriptor(const FFieldClass* InPropertyClass, FName InCppType = NAME_None)
		: PropertyClass(InPropertyClass)
		, CppType(InCppType)
	{}
	FPropertyValidatorDescriptor(const FFieldClass* InPropertyClass, FString InCppType)
		: PropertyClass(InPropertyClass)
		, CppType(InCppType)
	{}

	FORCEINLINE bool IsValid() const { return PropertyClass != nullptr; }
	FORCEINLINE FName GetCppType() const { return CppType; }
	FORCEINLINE const FFieldClass* GetPropertyClass() const { return PropertyClass; }

	bool Matches(const FProperty* Property) const;

private:

	const FFieldClass* PropertyClass = nullptr;
	FName CppType = NAME_None;
};

FORCEINLINE uint32 GetTypeHash(const FPropertyValidatorDescriptor& Descriptor)
{
	return HashCombine(GetTypeHash(Descriptor.GetCppType()), PointerHash(Descriptor.GetPropertyClass()));
}

FORCEINLINE bool operator==(const FPropertyValidatorDescriptor& Lhs, const FPropertyValidatorDescriptor& Rhs)
{
	return Lhs.GetPropertyClass() == Rhs.GetPropertyClass() && Lhs.GetCppType() == Rhs.GetCppType();
}

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
	FORCEINLINE FPropertyValidatorDescriptor GetDescriptor() const { return Descriptor; }
	
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

	FPropertyValidatorDescriptor Descriptor;
};
