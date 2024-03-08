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
	
	FPropertyValidationContext(const UPropertyValidatorSubsystem* OwningSubsystem, const UObject* InSourceObject)
		: Subsystem(OwningSubsystem)
		, SourceObject(InSourceObject)
	{ }
	
	FPropertyValidationResult MakeValidationResult() const;
	
	void PropertyFails(const FProperty* Property, const FText& DefaultFailureMessage, const FText& PropertyPrefix = FText::GetEmpty());

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

	FORCEINLINE void IsPropertyContainerValid(const void* Container, const UStruct* Struct)
	{
		Subsystem->IsPropertyContainerValidWithContext(Container, Struct, *this);		
	}

	FORCEINLINE void IsPropertyValid(const void* Container, const FProperty* Property)
	{
		Subsystem->IsPropertyValidWithContext(Container, Property, *this);
	}

	FORCEINLINE void IsPropertyValueValid(const void* Value, const FProperty* ParentProperty, const FProperty* ValueProperty)
	{
		Subsystem->IsPropertyValueValidWithContext(Value, ParentProperty, ValueProperty, *this);
	}

	FORCEINLINE const UObject* GetSourceObject() const
	{
		check(SourceObject.IsValid());
		return SourceObject.Get();
	}
	
private:

	FText MakeFullMessage(const FText& FailureMessage, const FText& PropertyPrefix) const;

	struct FIssue
	{
		FText Message;
		EMessageSeverity::Type Severity = EMessageSeverity::Info;
		const FProperty* IssueProperty = nullptr;
	};

	TArray<FIssue> Issues;
	TArray<FString> Prefixes;
	FString Context = "";
	TWeakObjectPtr<const UPropertyValidatorSubsystem> Subsystem;
	TWeakObjectPtr<const UObject> SourceObject;
};

struct FPropertyValidationResult
{
	FPropertyValidationResult() = default;
	FPropertyValidationResult(EDataValidationResult InResult)
		: ValidationResult(InResult)
	{ }
	
	TArray<FText> Errors;
	TArray<FText> Warnings;
	EDataValidationResult ValidationResult = EDataValidationResult::NotValidated;
};
