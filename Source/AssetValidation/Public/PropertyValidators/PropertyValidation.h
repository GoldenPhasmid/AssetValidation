#pragma once

#include "CoreMinimal.h"
#include "PropertyValidatorSubsystem.h"
#include "Templates/NonNullPointer.h"
#include "UObject/UObjectGlobals.h"


struct FPropertyValidationResult;
class FPropertyValidationContext;
class UPropertyValidatorSubsystem;

namespace UE::AssetValidation
{
	class FMetaDataSource;
	
	static const FName Validate("Validate");
	static const FName ValidateKey("ValidateKey");
	static const FName ValidateValue("ValidateValue");
	static const FName ValidateRecursive("ValidateRecursive");
	static const FName FailureMessage("FailureMessage");
}

namespace UE::AssetValidation
{
	/** @return true if "Validate" meta can be applied to given property */
	bool CanApplyMeta_Validate(const FProperty* Property);
	/** @return true if "ValidateRecursive" meta can be applied to given property */
	bool CanApplyMeta_ValidateRecursive(const FProperty* Property);
	/** @return true if "ValidateKey" meta can be applied to given property */
	bool CanApplyMeta_ValidateKey(const FProperty* Property);
	/** @return true if "ValidateValue" meta can be applied to given property */
	bool CanApplyMeta_ValidateValue(const FProperty* Property);
	/**
	 * @return true if @MetaName can be applied to @Property type
	 * Includes unwrapping container properties to check whether underlying type can be validated at all
	 */
	bool CanApplyMeta(const FProperty* Property, const FName& MetaName);
	/** @return true if property value validation meta can be added to a property */
	bool CanValidatePropertyValue(const FProperty* Property);
	/** @return true if property can be validated recursively */
	bool CanValidatePropertyRecursively(const FProperty* Property);

	/**
	 * Update single meta data key represented by @MetaName on variable defined by @Property
	 * @param Blueprint blueprint that associates with the variable
	 * @param Property variable property
	 * @param VarName variable name
	 * @param MetaName meta data key
	 * @param bAddIfPossible if set to true, update will rely only on property data. Otherwise meta data should be already present to stay
	 * @return whether meta data is present on property
	 */
	bool UpdateBlueprintVarMetaData(UBlueprint* Blueprint, const FProperty* Property, const FName& VarName, const FName& MetaName, bool bAddIfPossible);

	/** @return true if property is a container property (array, set or map) */
	bool IsContainerProperty(const FProperty* Property);
	
	/** @return whether package is a blueprint package */
	bool IsBlueprintGeneratedPackage(const FString& PackageName);
}

/**
 * Property Validation Context
 */
class FPropertyValidationContext: public FNoncopyable
{
public:
	
	FPropertyValidationContext(const UPropertyValidatorSubsystem* OwningSubsystem, const UObject* InSourceObject);

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
		check(!Prefix.IsEmpty());
		
		Prefixes.Push(Prefix);
		ContextString.Append(Prefix + TEXT("."));
	}

	/** pop last prefix from context string */
	FORCEINLINE void PopPrefix()
	{
		const FString Prefix = Prefixes.Pop();
		ContextString = ContextString.LeftChop(Prefix.Len() + 1);
	}

	/** Route property container validation request to validator subsystem */
	void IsPropertyContainerValid(TNonNullPtr<const uint8> ContainerMemory, const UStruct* Struct);
	/** Route property container validation request to validator subsystem, add scoped prefix */
	void IsPropertyContainerValid(TNonNullPtr<const uint8> ContainerMemory, const UStruct* Struct, const FString& ScopedPrefix);
	/** Route property validation request to validator subsystem */
	void IsPropertyValid(TNonNullPtr<const uint8> ContainerMemory, const FProperty* Property, UE::AssetValidation::FMetaDataSource& MetaData);
	/** Route property value validation request to validator subsystem */
	void IsPropertyValueValid(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, UE::AssetValidation::FMetaDataSource& MetaData);

	FORCEINLINE const UObject* GetSourceObject() const
	{
		check(SourceObject.IsValid());
		return SourceObject.Get();
	}

private:
	
	FText MakeFullMessage(const FText& FailureMessage, const FText& PropertyPrefix) const;
	/** @return beautified name for an object */
    FString GetBeautifiedName(const UObject* Object) const;
	
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




