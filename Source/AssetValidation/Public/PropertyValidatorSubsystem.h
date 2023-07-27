#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"

#include "PropertyValidatorSubsystem.generated.h"

class UPropertyValidatorBase;
struct FPropertyValidationResult;

namespace ValidationNames
{
	static const FName Validate("Validate");
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
public:

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;


	/**
	 * @brief 
	 * @param Container 
	 * @param Struct 
	 * @param ValidationResult 
	 */
	virtual void IsPropertyContainerValid(void* Container, UStruct* Struct, FPropertyValidationResult& ValidationResult) const;
	
	/**
	 * @brief 
	 * @param Container 
	 * @param Property
	 * @param ValidationResult
	 * @return 
	 */
	virtual void IsPropertyValid(void* Container, FProperty* Property, FPropertyValidationResult& ValidationResult) const;

	/**
	 * @brief 
	 * @param Value 
	 * @param ParentProperty 
	 * @param ValueProperty 
	 * @return 
	 */
	virtual void IsPropertyValueValid(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationResult& ValidationResult);

protected:

	bool CanValidatePackage(UPackage* Package) const;

	bool IsBlueprintGenerated(UPackage* Package) const;

	UPROPERTY(Config)
	TArray<FString> PackagesToValidate;

	UPROPERTY(Config)
	bool bSkipBlueprintGeneratedClasses = false;
	
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPropertyValidatorBase>> Validators;
};
