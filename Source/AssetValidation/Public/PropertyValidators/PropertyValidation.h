#pragma once

#include "CoreMinimal.h"
#include "PropertyValidatorSubsystem.h"
#include "Experimental/Coroutine/Coroutine.h"
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

	FORCEINLINE void FailOnCondition(bool Condition, const FProperty* Property, const FText& DefaultFailureMessage)
	{
		if (Condition)
		{
			PropertyFails(Property, DefaultFailureMessage);
		}
	}
	
	void PropertyFails(const FProperty* Property, const FText& DefaultFailureMessage);

	/** push prefix to context string */
	FORCEINLINE void PushPrefix(const FString& Prefix)
	{
		Prefixes.Push(Prefix);
		ContextString.Append(Prefix + TEXT("."));
	}

	/** pop last prefix from context string */
	FORCEINLINE void PopPrefix()
	{
		const FString Prefix = Prefixes.Pop();
		ContextString = ContextString.LeftChop(Prefix.Len() + 1);
	}

	FORCEINLINE void IsPropertyContainerValid(TNonNullPtr<const uint8> ContainerMemory, const UStruct* Struct)
	{
		Subsystem->ValidateContainerWithContext(ContainerMemory, Struct, *this);		
	}

	FORCEINLINE void IsPropertyValid(TNonNullPtr<const uint8> ContainerMemory, const FProperty* Property)
	{
		Subsystem->ValidatePropertyWithContext(ContainerMemory, Property, *this);
	}

	FORCEINLINE void IsPropertyValueValid(TNonNullPtr<const uint8> PropertyMemory, const FProperty* ParentProperty, const FProperty* ValueProperty)
	{
		Subsystem->ValidatePropertyValueWithContext(PropertyMemory, ParentProperty, ValueProperty, *this);
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
	/** Stack of prefixes appended to @ContextString */
	TArray<FString> Prefixes;
	/**
	 * Context string that is added to "property fails" error message.
	 * Allows to understand property hierarchies for nested structs/arrays/objects
	 */
	FString ContextString = "";
	/** Weak reference to property validator subsystem */
	TWeakObjectPtr<const UPropertyValidatorSubsystem> Subsystem;
	/** Weak reference to an initial object from which validation sequence has started */
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

template <typename TPropertyType>
struct TPropertyTypeResolverImpl
{
	using Type = typename TPropertyType::TCppType;
};

template <typename TPropertyType>
typename TPropertyType::TCppType GetPropertyValue(const void* PropertyMemory, const FProperty* Property)
{
	return CastFieldChecked<TPropertyType>(Property)->GetPropertyValue(PropertyMemory);
}

template <typename TPropertyType>
const typename TPropertyType::TCppType* GetPropertyValuePtr(const void* PropertyMemory, const FProperty* Property)
{
	return CastFieldChecked<TPropertyType>(Property)->GetPropertyValuePtr(PropertyMemory);
}




