#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "PropertyValidators/PropertyValidationResult.h"
#include "Templates/NonNullPointer.h"

#include "PropertyValidatorSubsystem.generated.h"

class FFieldClass;
class UPropertyValidatorBase;
class FPropertyValidationContext;
struct FPropertyValidationResult;

/**
 *
 */
UCLASS(Config = Editor)
class ASSETVALIDATION_API UPropertyValidatorSubsystem: public UEditorSubsystem
{
	GENERATED_BODY()

	friend class FPropertyValidationContext;
public:
	
	/** @return true if property is a container property (array, set or map) */
	static bool IsContainerProperty(const FProperty* Property);

	//~Begin USubsystem interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~End USubsystem interface

	/**
	 * @return validation result for given object
	 * @param Object object to perform full validation on
	 */
	FPropertyValidationResult ValidateObject(const UObject* Object) const;

	/**
	 * @return property validation result for given struct inside another object
	 * @param OwningObject logically owns script struct (doesn't mean that struct data is a part of object's memory) and indicates object of validation
	 * @param ScriptStruct struct type to perform full validation
	 * @param StructData memory that represents @ScriptStruct
	 */
	FPropertyValidationResult ValidateStruct(const UObject* OwningObject, const UScriptStruct* ScriptStruct, const uint8* StructData);

	template <typename TStructType>
	FPropertyValidationResult ValidateStruct(const UObject* OwningObject, const TStructType& Value)
	{
		return ValidateStruct(OwningObject, TStructType::StaticStruct(), reinterpret_cast<const uint8*>(&Value));
	}

	/**
	 * @return validation result for given property that is part of given object
	 * @param Object object that property is part of and located in
	 * @param Property property to validate
	 */
	FPropertyValidationResult ValidateObjectProperty(const UObject* Object, const FProperty* Property) const;

	/**
	 * @return validation result for given property that belongs to struct type and stored inside struct data
	 * @param OwningObject logically owns script struct (doesn't mean that struct data is a part of object's memory) and indicates object of validation
	 * @param ScriptStruct struct type given property belongs to
	 * @param Property property to validate
	 * @param StructData a region of memory that holds @ScriptStruct object type
	 */
	FPropertyValidationResult ValidateStructProperty(const UObject* OwningObject, const UScriptStruct* ScriptStruct, FProperty* Property, const uint8* StructData);

	template <typename TStructType>
	FPropertyValidationResult ValidateStructProperty(const UObject* OwningObject, FProperty* Property, const TStructType& Value)
	{
		return ValidateStructProperty(OwningObject, TStructType::StaticStruct(), Property, reinterpret_cast<const uint8*>(&Value));
	}

	/** @return whether validator subsystem can run validators on a given object */
	bool CanValidatePackage(const UPackage* Package) const;

	/** @return whether subsystem can validate following property type */
	bool HasValidatorForPropertyValue(const FProperty* PropertyType) const;

	/** @return whether property can be ever validated based on its property flags */
	bool CanEverValidateProperty(const FProperty* Property) const;

protected:
	
	/**
	 * @brief validate all properties in @Container
	 * @param ContainerMemory container to get data from
	 * @param Struct struct to retrieve
	 * @param ValidationContext provided validation context
	 */
	virtual void ValidateContainerWithContext(TNonNullPtr<const uint8> ContainerMemory, const UStruct* Struct, FPropertyValidationContext& ValidationContext) const;

	/**
	 * @brief validate @Property in @Container
	 * @param ContainerMemory container to get data from
	 * @param Property property to validate
	 * @param ValidationContext provided validation context
	 */
	virtual void ValidatePropertyWithContext(TNonNullPtr<const uint8> ContainerMemory, const FProperty* Property, FPropertyValidationContext& ValidationContext) const;
	
	/**
	 * @brief validate given property value
	 * @param PropertyMemory property value
	 * @param ParentProperty parent property, typically a container property where property value is stored
	 * @param ValueProperty value property 
	 * @param ValidationContext provided validation context
	 */
	virtual void ValidatePropertyValueWithContext(TNonNullPtr<const uint8> PropertyMemory, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const;
	
	/** @return whether property should be validated for given @ValidationContext */
	bool ShouldValidateProperty(const FProperty* Property, FPropertyValidationContext& ValidationContext) const;

	/** @return whether package is a blueprint generated class */
	bool IsBlueprintGenerated(const UPackage* Package) const;

	/** @return property validator for a given property type */
	const UPropertyValidatorBase* FindPropertyValidator(const FProperty* PropertyType) const;
	/** @return container  validator for a given property type */
	const UPropertyValidatorBase* FindContainerValidator(const FProperty* PropertyType) const;

	UPROPERTY(Config)
	TArray<FString> PackagesToValidate;

	UPROPERTY(Config)
	bool bSkipBlueprintGeneratedClasses = false;
	
	TMap<FFieldClass*, UPropertyValidatorBase*> ContainerValidators;
	TMap<FFieldClass*, UPropertyValidatorBase*> PropertyValidators;
	TMap<FString, UPropertyValidatorBase*> StructValidators;

	UPROPERTY(Transient)
	TArray<UPropertyValidatorBase*> AllValidators;
};
