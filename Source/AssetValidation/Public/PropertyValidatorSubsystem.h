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

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * @brief 
	 * @param InObject 
	 * @param Property 
	 * @return 
	 */
	virtual FPropertyValidationResult IsPropertyValid(UObject* InObject, FProperty* Property) const;

	/**
	 * @brief 
	 * @param Value 
	 * @param ParentProperty 
	 * @param ValueProperty 
	 * @return 
	 */
	virtual void IsPropertyValueValid(void* Value, FProperty* ParentProperty, FProperty* ValueProperty, FPropertyValidationResult& ValidationResult);

protected:

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPropertyValidatorBase>> Validators;
};
