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
	/**
	 * Property validation meta specifiers
	 * see Tests/MetaSpecifierTests.cpp/FAutomationTest_MetaSpecifiers::RunTest for more info
	 */
	static const FName Validate("Validate");
	static const FName ValidateKey("ValidateKey");
	static const FName ValidateValue("ValidateValue");
	static const FName ValidateRecursive("ValidateRecursive");
	static const FName FailureMessage("FailureMessage");
	static const FName DisableEditOnTemplate("DisableEditOnTemplate");

	ASSETVALIDATION_API const TStaticArray<FName, 6>& GetMetaKeys();
}

namespace UE::AssetValidation
{
	/** */
	ASSETVALIDATION_API bool PassesEditCondition(UStruct* Struct, TNonNullPtr<const uint8> Container, const FProperty* Property);
	/**
	 * checks property meta data to see if any meta specifiers are placed incorrectly
	 * @return true if all metas can be applied to a property, false otherwise
	 */
	ASSETVALIDATION_API bool CheckPropertyMetaData(const FProperty* Property, const FMetaDataSource& MetaData, bool bLoggingEnabled);
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
	ASSETVALIDATION_API bool CanApplyMeta(const FProperty* Property, const FName& MetaName);
	/** @return true if property value validation meta can be added to a property */
	ASSETVALIDATION_API bool CanValidatePropertyValue(const FProperty* Property);
	/** @return true if property can be validated recursively */
	ASSETVALIDATION_API bool CanValidatePropertyRecursively(const FProperty* Property);
	/** @return true if @Property an actor component with owner being a blueprint class */
	ASSETVALIDATION_API bool IsBlueprintComponentProperty(const FProperty* Property);
	/** @return whether property is visible in blueprints */
	ASSETVALIDATION_API bool IsBlueprintVisibleProperty(const FProperty* Property);
	/** @return property display name set by user */
	ASSETVALIDATION_API FString GetPropertyDisplayName(const FProperty* Property);
	/** @return property underlying type name */
	ASSETVALIDATION_API FString GetPropertyTypeName(const FProperty* Property);
	/** @return object display name */
	ASSETVALIDATION_API FString ResolveObjectDisplayName(const UObject* Object, FPropertyValidationContext& ValidationContext);

	/**
	 * Update single meta data key represented by @MetaName on variable defined by @Property
	 * @param Blueprint blueprint that associates with the variable
	 * @param Property variable property
	 * @param VarName variable name
	 * @param MetaName meta data key
	 * @param bAddIfPossible if set to true, update will rely only on property data. Otherwise meta data should be already present to stay
	 * @return whether meta data is present on property
	 */
	ASSETVALIDATION_API bool UpdateBlueprintVarMetaData(UBlueprint* Blueprint, const FProperty* Property, const FName& VarName, const FName& MetaName, bool bAddIfPossible);

	/** @return true if property is a container property (array, set or map) */
	ASSETVALIDATION_API bool IsContainerProperty(const FProperty* Property);
	
	/** @return whether package is a blueprint package */
	ASSETVALIDATION_API bool IsBlueprintGeneratedPackage(const FString& PackageName);
}

/**
 * Property Validation Context
 */
class ASSETVALIDATION_API FPropertyValidationContext: public FNoncopyable
{
public:
	
	/** Scoped prefix struct */
	class FScopedPrefix
	{
	public:
		FScopedPrefix(FPropertyValidationContext& InContext, const FString& Prefix)
			: Context(InContext)
		{
			Context.PushPrefix(Prefix);
		}
		
		~FScopedPrefix()
		{
			Context.PopPrefix();
		}
	private:
		FPropertyValidationContext& Context;
	};
	
	/** Scoped conditional prefix */
	class FConditionalPrefix
	{
	public:
		FConditionalPrefix(FPropertyValidationContext& InContext, const FString& Prefix, bool bCondition)
			: Context(InContext)
			, bPushed(bCondition)
		{
			if (bPushed)
			{
				Context.PushPrefix(Prefix);
			}
		}

		~FConditionalPrefix()
		{
			if (bPushed)
			{
				Context.PopPrefix();
			}
		}
	private:
		FPropertyValidationContext& Context;
		bool bPushed = false;
	};
	
	/** Scoped object struct */
	class FScopedSourceObject
	{
	public:
		FScopedSourceObject(FPropertyValidationContext& InContext, const UObject* InObject)
			: Context(InContext)
		{
			Context.PushSource(InObject);
		}
		~FScopedSourceObject()
		{
			Context.PopSource();
		}

	private:
		FPropertyValidationContext& Context;
	};
	
	FPropertyValidationContext(const UPropertyValidatorSubsystem* OwningSubsystem, const UObject* InSourceObject);

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
	
	/** @return last pushed prefix */
	FORCEINLINE FString GetPrefix() const
	{
		check(Prefixes.Num() > 0);
		return Prefixes.Last();
	}

	/** pop last prefix from context string */
	FORCEINLINE void PopPrefix()
	{
		check(Prefixes.Num() > 0);
		const FString Prefix = Prefixes.Pop();
		ContextString = ContextString.LeftChop(Prefix.Len() + 1);
	}

	/** */
	FPropertyValidationResult MakeValidationResult() const;
	/** Route property container validation request to validator subsystem */
	void IsPropertyContainerValid(TNonNullPtr<const uint8> ContainerMemory, const UStruct* Struct);
	/** Route property validation request to validator subsystem */
	void IsPropertyValid(TNonNullPtr<const uint8> ContainerMemory, const FProperty* Property, UE::AssetValidation::FMetaDataSource& MetaData);
	/** Route property value validation request to validator subsystem */
	void IsPropertyValueValid(TNonNullPtr<const uint8> PropertyMemory, const FProperty* Property, UE::AssetValidation::FMetaDataSource& MetaData);

	FORCEINLINE const UObject* GetSourceObject() const
	{
		check(Objects.Num() > 0);
		return Objects.Last().Get();
	}

private:

	FORCEINLINE void PushSource(const UObject* InObject)
	{
		check(IsValid(InObject));
		Objects.Push(InObject);
	}

	FORCEINLINE void PopSource()
	{
		check(Objects.Num() > 0);
		Objects.Pop();
	}
	
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
	/** Weak reference to the object chain, starting from which validation sequence has started */
	TArray<TWeakObjectPtr<const UObject>> Objects;
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

template <typename T>
static FString GetStructCppName()
{
	return TBaseStructure<T>::Get()->GetStructCPPName();
}

template <typename T>
static const T* ConvertStructMemory(const uint8* Memory)
{
	return static_cast<const T*>((const void*)Memory);
}
