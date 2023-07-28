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
	

	FPropertyValidationResult IsPropertyContainerValid(UObject* Object) const;
	
	FPropertyValidationResult IsPropertyValid(UObject* Object, FProperty* Property) const;

protected:
	
	virtual void IsPropertyContainerValid(void* Container, UStruct* Struct, FPropertyValidationContext& ValidationContext) const;
	virtual void IsPropertyValid(void* Container, FProperty* Property, FPropertyValidationContext& ValidationContext) const;
	virtual void IsPropertyValueValid(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationContext& ValidationContext) const;
	
	bool CanValidatePackage(UPackage* Package) const;

	bool IsBlueprintGenerated(UPackage* Package) const;

	UPROPERTY(Config)
	TArray<FString> PackagesToValidate;

	UPROPERTY(Config)
	bool bSkipBlueprintGeneratedClasses = false;
	
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPropertyValidatorBase>> Validators;
	
	TMap<FFieldClass*, UPropertyValidatorBase*> GroupedValidators;
};
