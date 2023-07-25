#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"

#include "PropertyValidatorSubsystem.generated.h"

class UPropertyValidatorBase;
struct FPropertyValidationResult;

UCLASS(Config = Editor)
class ASSETVALIDATION_API UPropertyValidatorSubsystem: public UEditorSubsystem
{
	GENERATED_BODY()
public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual FPropertyValidationResult IsPropertyValid(UObject* InObject, FProperty* Property) const;

protected:

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPropertyValidatorBase>> Validators;
};
