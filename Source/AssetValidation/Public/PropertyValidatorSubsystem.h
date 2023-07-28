#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"

#include "PropertyValidatorSubsystem.generated.h"

class FFieldClass;
class UPropertyValidatorBase;
class FPropertyValidationContext;
struct FPropertyValidationResult;

namespace ValidationNames
{
	static const FName Validate("Validate");
	static const FName ValidateKey("ValidateKey");
	static const FName ValidateValue("ValidateValue");
	static const FName ValidateWarning("ValidateWarning");
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
	FPropertyValidationResult IsPropertyContainerValid(UObject* Object) const;

	/** @return property validation result for given @Property for @Object */
	FPropertyValidationResult IsPropertyValid(UObject* Object, FProperty* Property) const;

protected:
	
	/**
	 * @brief validate all properties in @Container
	 * @param Container container to get data from
	 * @param Struct struct to retrieve
	 * @param ValidationContext provided validation context
	 */
	virtual void IsPropertyContainerValid(void* Container, UStruct* Struct, FPropertyValidationContext& ValidationContext) const;

	/**
	 * @brief validate @Property in @Container
	 * @param Container container to get data from
	 * @param Property property to validate
	 * @param ValidationContext provided validation context
	 */
	virtual void IsPropertyValid(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const;

	/**
	 * @brief validate given property value
	 * @param Value property value
	 * @param ParentProperty parent property, typically a container property where property value is stored
	 * @param ValueProperty value property 
	 * @param ValidationContext provided validation context
	 */
	virtual void IsPropertyValueValid(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const;

	/** @return whether package should be validated */
	bool ShouldValidatePackage(UPackage* Package) const;

	/** @return whether package is a blueprint generated class */
	bool IsBlueprintGenerated(UPackage* Package) const;

	UPROPERTY(Config)
	TArray<FString> PackagesToValidate;

	UPROPERTY(Config)
	bool bSkipBlueprintGeneratedClasses = false;
	
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPropertyValidatorBase>> Validators;
	
	TMap<FFieldClass*, UPropertyValidatorBase*> GroupedValidators;
};
