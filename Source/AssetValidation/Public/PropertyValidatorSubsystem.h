#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"

#include "PropertyValidatorSubsystem.generated.h"

class FFieldClass;
class UPropertyValidatorBase;
class UPropertyContainerValidator;
class FPropertyValidationContext;
struct FPropertyValidationResult;

namespace ValidationNames
{
	static const FName Validate("Validate");
	static const FName ValidateKey("ValidateKey");
	static const FName ValidateValue("ValidateValue");
	static const FName ValidateRecursive("ValidateRecursive");
	static const FName ValidationFailureMessage("FailureMessage");
};

/**
 *
 */
UCLASS(Config = Editor)
class ASSETVALIDATION_API UPropertyValidatorSubsystem: public UEditorSubsystem
{
	GENERATED_BODY()

	friend class FPropertyValidationContext;
	
public:

	//~Begin USubsystem interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~End USubsystem interface

	/** @return property validation result for given @Object */
	FPropertyValidationResult IsPropertyContainerValid(const UObject* Object) const;

	/** @return property validation result for given struct data */
	FPropertyValidationResult IsPropertyContainerValid(const UObject* OwningObject, const UScriptStruct* ScriptStruct, void* StructData);

	/** @return property validation result for given @Property for @Object */
	FPropertyValidationResult IsPropertyValid(const UObject* Object, FProperty* Property) const;

	/** @return property validation result property in given struct data */
	FPropertyValidationResult IsPropertyValid(const UObject* OwningObject, const UScriptStruct* ScriptStruct, FProperty* Property, void* StructData);

protected:
	
	/**
	 * @brief validate all properties in @Container
	 * @param Container container to get data from
	 * @param Struct struct to retrieve
	 * @param ValidationContext provided validation context
	 */
	virtual void IsPropertyContainerValidWithContext(const void* Container, const UStruct* Struct, FPropertyValidationContext& ValidationContext) const;

	/**
	 * @brief validate @Property in @Container
	 * @param Container container to get data from
	 * @param Property property to validate
	 * @param ValidationContext provided validation context
	 */
	virtual void IsPropertyValidWithContext(const void* Container, const FProperty* Property, FPropertyValidationContext& ValidationContext) const;
	
	/**
	 * @brief validate given property value
	 * @param Value property value
	 * @param ParentProperty parent property, typically a container property where property value is stored
	 * @param ValueProperty value property 
	 * @param ValidationContext provided validation context
	 */
	virtual void IsPropertyValueValidWithContext(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const;

	/** @return whether package should be validated for given @ValidationContext */
	virtual bool ShouldValidatePackage(UPackage* Package, FPropertyValidationContext& ValidationContext) const;

	/** @return whether property should be validated for given @ValidationContext */
	virtual bool ShouldValidateProperty(const FProperty* Property, FPropertyValidationContext& ValidationContext) const;

	/** @return whether package is a blueprint generated class */
	bool IsBlueprintGenerated(UPackage* Package) const;

	UPROPERTY(Config)
	TArray<FString> PackagesToValidate;

	UPROPERTY(Config)
	bool bSkipBlueprintGeneratedClasses = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPropertyContainerValidator>> ContainerValidators;
	
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPropertyValidatorBase>> PropertyValidators;
};
