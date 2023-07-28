#pragma once

#include "CoreMinimal.h"
#include "PropertyValidatorSubsystem.h"
#include "UObject/UObjectGlobals.h"

class UPropertyValidatorSubsystem;
class FPropertyValidationContext;
struct FPropertyValidationResult;

class FPropertyValidationContext: public FNoncopyable
{
public:
	
	FPropertyValidationContext(const UPropertyValidatorSubsystem* OwningSubsystem)
		: Subsystem(OwningSubsystem)
	{ }
	
	FPropertyValidationResult MakeValidationResult() const;
	
	void PropertyFails(FProperty* Property, const FText& DefaultFailureMessage, const FText& PropertyPrefix = FText::GetEmpty());

	FORCEINLINE void PushPrefix(const FString& Prefix)
	{
		Prefixes.Push(Prefix);
		Context.Append(Prefix + TEXT("."));
	}

	FORCEINLINE void PopPrefix()
	{
		const FString Prefix = Prefixes.Pop();
		Context = Context.LeftChop(Prefix.Len() + 1);
	}

	FORCEINLINE void IsPropertyContainerValid(void* Container, UStruct* Struct)
	{
		Subsystem->IsPropertyContainerValid(Container, Struct, *this);		
	}

	FORCEINLINE void IsPropertyValid(void* Container, FProperty* Property)
	{
		Subsystem->IsPropertyValid(Container, Property, *this);
	}

	FORCEINLINE void IsPropertyValueValid(void* Value, FProperty* ParentProperty, FProperty* ValueProperty)
	{
		Subsystem->IsPropertyValueValid(Value, ParentProperty, ValueProperty, *this);
	}

private:

	FText MakeFullMessage(const FText& FailureMessage, const FText& PropertyPrefix) const;

	struct FIssue
	{
		FText Message;
		EMessageSeverity::Type Severity = EMessageSeverity::Info;
		FProperty* IssueProperty = nullptr;
	};

	TArray<FIssue> Issues;
	TArray<FString> Prefixes;
	FString Context = "";
	TWeakObjectPtr<const UPropertyValidatorSubsystem> Subsystem;
};

struct FPropertyValidationResult
{
	TArray<FText> Errors;
	TArray<FText> Warnings;
	EDataValidationResult ValidationResult = EDataValidationResult::NotValidated;
};
