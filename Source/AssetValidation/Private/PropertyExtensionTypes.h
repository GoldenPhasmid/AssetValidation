﻿#pragma once

#include "PropertyExtensionTypes.generated.h"

class UObject;
class UClass;
class UStruct;
class UUserDefinedStruct;
struct FPropertyExtensionConfig;
struct FEnginePropertyExtension;

/**
 * Single property meta data extension, in config format
 */
USTRUCT(BlueprintType)
struct ASSETVALIDATION_API FPropertyExtensionConfig
{
	GENERATED_BODY()

	FPropertyExtensionConfig() = default;
	FPropertyExtensionConfig(const FEnginePropertyExtension& Extension);

	/** class path in a /<PackageName>/<ModuleName>.<ClassName> format */
	UPROPERTY(EditAnywhere, meta = (AllowAbstract = "true"))
	FSoftObjectPath Class;

	/** Simple property name */
	UPROPERTY(EditAnywhere)
	FName Property = NAME_None;

	/** property meta data, list of pairs Key=Value separated by ';' */
	UPROPERTY(EditAnywhere)
	FString MetaData;

	/** user comment */
	UPROPERTY(EditAnywhere)
	FString Comment;
};

/**
 * Single property meta data extension
 */
USTRUCT()
struct ASSETVALIDATION_API FEnginePropertyExtension
{
	GENERATED_BODY()
	
	FEnginePropertyExtension() = default;
	FEnginePropertyExtension(UStruct* InStruct, const TFieldPath<FProperty>& InPropertyPath)
		: Struct(InStruct)
		, PropertyPath(InPropertyPath)
	{}
	FEnginePropertyExtension(const FPropertyExtensionConfig& Config);

	FORCEINLINE bool IsValid() const
	{
		return Struct != nullptr && GetProperty() != nullptr;
	}
	
	FORCEINLINE FProperty* GetProperty() const
	{
		return PropertyPath.Get(Struct);
	}

	FORCEINLINE bool HasMetaData(const FName& Key) const
	{
		return MetaDataMap.Find(Key) != nullptr;
	}

	FORCEINLINE FString GetMetaData(const FName& Key) const
	{
		if (const FString* Str = MetaDataMap.Find(Key))
		{
			return *Str;
		}

		return FString{};
	}

	FORCEINLINE void SetMetaData(const FName& Key, const FString& Value)
	{
		MetaDataMap.Add(Key, Value);
	}

	FORCEINLINE void RemoveMetaData(const FName& Key)
	{
		MetaDataMap.Remove(Key);
	}

	UPROPERTY(EditAnywhere)
	TObjectPtr<UStruct> Struct = nullptr;

	UPROPERTY(EditAnywhere)
	TFieldPath<FProperty> PropertyPath;

	UPROPERTY(EditAnywhere)
	TMap<FName, FString> MetaDataMap;
};

/**
 * Class extension data
 */
USTRUCT()
struct ASSETVALIDATION_API FEngineClassExtension
{
	GENERATED_BODY()

	FEngineClassExtension() = default;
	FEngineClassExtension(UClass* InClass)
		: Class(InClass)
	{}

	FORCEINLINE bool IsValid() const
	{
		return Class != nullptr;
	}

	UPROPERTY(EditAnywhere, meta = (AllowAbstract = "true"))
	TSubclassOf<UObject> Class;

	UPROPERTY(EditAnywhere)
	TArray<FEnginePropertyExtension> Properties;
};

FORCEINLINE bool operator==(const FEngineClassExtension& ClassData, const UClass* Class)
{
	return ClassData.Class == Class;
}

/**
 * Extension data for a script struct
 */
USTRUCT()
struct ASSETVALIDATION_API FEngineStructExtension
{
	GENERATED_BODY()

	FEngineStructExtension() = default;
	FEngineStructExtension(UScriptStruct* InStruct)
		: Struct(InStruct)
	{}

	FORCEINLINE bool IsValid() const
	{
		return Struct != nullptr;
	}

	UPROPERTY(EditAnywhere)
	TObjectPtr<UScriptStruct> Struct;

	UPROPERTY(EditAnywhere)
	TArray<FEnginePropertyExtension> Properties;
};


FORCEINLINE bool operator==(const FEngineStructExtension& StructData, const UScriptStruct* ScriptStruct)
{
	return StructData.Struct == ScriptStruct;
}


