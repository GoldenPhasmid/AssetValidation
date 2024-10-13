#pragma once

#include "PropertyExtensionTypes.generated.h"

class UObject;
class UClass;
class UStruct;
class UUserDefinedStruct;
struct FPropertyMetaDataExtensionConfig;
struct FPropertyMetaDataExtension;

/**
 * Single property meta data extension, in config format
 */
USTRUCT()
struct ASSETVALIDATION_API FPropertyMetaDataExtensionConfig
{
	GENERATED_BODY()

	FPropertyMetaDataExtensionConfig() = default;
	FPropertyMetaDataExtensionConfig(const FPropertyMetaDataExtension& Extension);

	/** class path in a /<PackageName>/<ModuleName>.<ClassName> format */
	UPROPERTY()
	FSoftObjectPath Class;

	/** Simple property name */
	UPROPERTY()
	FName Property = NAME_None;

	/** property meta data, list of pairs Key=Value separated by ';' */
	UPROPERTY()
	FString MetaData;

	/** user comment */
	UPROPERTY()
	FString Comment;
};

/**
 * Single property meta data extension
 */
USTRUCT()
struct ASSETVALIDATION_API FPropertyMetaDataExtension
{
	GENERATED_BODY()
	
	FPropertyMetaDataExtension() = default;
	FPropertyMetaDataExtension(UStruct* InStruct, const TFieldPath<FProperty>& InPropertyPath)
		: Struct(InStruct)
		, PropertyPath(InPropertyPath)
	{}
	explicit FPropertyMetaDataExtension(const FPropertyMetaDataExtensionConfig& Config);

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

	UPROPERTY(EditAnywhere, meta = (Validate))
	TObjectPtr<UStruct> Struct = nullptr;

	UPROPERTY(EditAnywhere, meta = (Validate))
	TFieldPath<FProperty> PropertyPath;

	UPROPERTY(EditAnywhere, meta = (ValidateKey))
	TMap<FName, FString> MetaDataMap;
};

/**
 * Property extension data for a class
 */
USTRUCT()
struct ASSETVALIDATION_API FUClassMetaDataExtension
{
	GENERATED_BODY()

	FUClassMetaDataExtension() = default;
	explicit FUClassMetaDataExtension(UClass* InClass)
		: Class(InClass)
	{}

	FORCEINLINE bool IsValid() const
	{
		return Class != nullptr;
	}

	UPROPERTY(EditAnywhere, meta = (Validate, AllowAbstract = "true"))
	TSubclassOf<UObject> Class;

	UPROPERTY(EditAnywhere)
	TArray<FPropertyMetaDataExtension> Properties;
};

FORCEINLINE bool operator==(const FUClassMetaDataExtension& ClassData, const UClass* Class)
{
	return ClassData.Class == Class;
}

/**
 * Property extension data for a script struct
 */
USTRUCT()
struct ASSETVALIDATION_API FUScriptStructMetaDataExtension
{
	GENERATED_BODY()

	FUScriptStructMetaDataExtension() = default;
	explicit FUScriptStructMetaDataExtension(UScriptStruct* InStruct)
		: Struct(InStruct)
	{}

	FORCEINLINE bool IsValid() const
	{
		return Struct != nullptr;
	}

	UPROPERTY(EditAnywhere, meta = (Validate, AllowAbstract = "true"))
	TObjectPtr<UScriptStruct> Struct;

	UPROPERTY(EditAnywhere)
	TArray<FPropertyMetaDataExtension> Properties;
};


FORCEINLINE bool operator==(const FUScriptStructMetaDataExtension& StructData, const UScriptStruct* ScriptStruct)
{
	return StructData.Struct == ScriptStruct;
}

UCLASS()
class ASSETVALIDATION_API UPropertyMetaDataExtensionSet: public UDataAsset
{
	GENERATED_BODY()
public:

	static FSimpleDelegate OnPropertyMetaDataChanged;
	
	virtual void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;

	void FillPropertyMap(TMap<FSoftObjectPath, TArray<FPropertyMetaDataExtension>>& ExtensionMap);
	
	/** List of classes with additional validation meta data */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (TitleProperty = "Class"))
	TArray<FUClassMetaDataExtension> ClassExtensions;

	/** List of structs with additional validation meta data */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (TitleProperty = "Struct"))
	TArray<FUScriptStructMetaDataExtension> StructExtensions;
};

