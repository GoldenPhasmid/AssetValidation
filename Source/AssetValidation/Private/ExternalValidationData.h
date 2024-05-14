#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "ExternalValidationData.generated.h"

USTRUCT()
struct FPropertyExternalValidationData
{
	GENERATED_BODY()
	
	FPropertyExternalValidationData() = default;
	FPropertyExternalValidationData(UStruct* InStruct, const TFieldPath<FProperty>& InPropertyPath)
		: Struct(InStruct)
		, PropertyPath(InPropertyPath)
	{}

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

USTRUCT()
struct FClassExternalValidationData
{
	GENERATED_BODY()

	FClassExternalValidationData() = default;
	FClassExternalValidationData(UClass* InClass)
		: Class(InClass)
	{}

	UPROPERTY(EditAnywhere, meta = (AllowAbstract = "true"))
	TSubclassOf<UObject> Class;

	UPROPERTY(EditAnywhere)
	TArray<FPropertyExternalValidationData> Properties;
};

FORCEINLINE bool operator==(const FClassExternalValidationData& ClassData, const UClass* Class)
{
	return ClassData.Class == Class;
}

USTRUCT()
struct FStructExternalValidationData
{
	GENERATED_BODY()

	FStructExternalValidationData() = default;
	FStructExternalValidationData(UScriptStruct* InStruct)
		: StructClass(InStruct)
	{}

	UPROPERTY(EditAnywhere)
	TObjectPtr<UScriptStruct> StructClass;

	UPROPERTY(EditAnywhere)
	TArray<FPropertyExternalValidationData> Properties;
};

UCLASS()
class ASSETVALIDATION_API UExternalValidationData: public UDeveloperSettings
{
	GENERATED_BODY()
public:

	/** List of classes with additional validation meta data */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	TArray<FClassExternalValidationData> ExternalClasses;

	/** List of structs with additional validation meta data */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	TArray<FStructExternalValidationData> ExternalStructs;

	
};
