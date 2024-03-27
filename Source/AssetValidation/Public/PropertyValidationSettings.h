#pragma once

#include "CoreMinimal.h"

#include "PropertyValidationSettings.generated.h"

class UClassExternalMetaData;

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

	UPROPERTY(EditAnywhere)
	TObjectPtr<UScriptStruct> StructClass;

	UPROPERTY(EditAnywhere)
	TArray<FPropertyExternalValidationData> Properties;
};

FORCEINLINE bool operator==(const FStructExternalValidationData& StructData, const UScriptStruct* ScriptStruct)
{
	return StructData.StructClass == ScriptStruct;
}

class FExternalValidationData: public TArray<FPropertyExternalValidationData>
{
	
};

UCLASS(Config = Editor, DefaultConfig, DisplayName = "Property Validation")
class ASSETVALIDATION_API UPropertyValidationSettings: public UDeveloperSettings
{
	GENERATED_BODY()
public:

	FORCEINLINE static const UPropertyValidationSettings* Get()
	{
		return GetDefault<UPropertyValidationSettings>();
	}

	static const TArray<FPropertyExternalValidationData>& GetExternalValidationData(const UStruct* Struct);
	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/**
	 * List of packages asset property validator will ignore
	 * Can be any content that doesn't have properties (e.g. /Game/Meshes, /Game/Materials, etc.)
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Settings", meta = (Validate))
	TArray<FString> PackagesToIgnore;

	/**
	 * List of packages that should be validated using class property iteration
	 * Include any C++/Blueprint packages that your team can edit (project, project plugins, blueprints)
	 * For validating packages that you don't have edit access (engine code & engine plugins), see @ExternalClasses/@ExternalStructs
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Settings", meta = (Validate))
	TArray<FString> PackagesToIterate;

#if 0
	/**
	 * List of additional class external meta data, neatly grouped together
	 * If adding external validation meta data for a particular class, consider using UClassExternalMetaData data asset
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	TArray<TSoftObjectPtr<UClassExternalMetaData>> ExternalMetaData;
#endif
	
	/**
	 * if set to true, allows unwrapping and iterating over struct inner properties without "ValidateRecursive" meta specifier
	 * Desired default behavior. It means users will require placing any metas on struct properties only if they want to validate an actual value, e.g. FGameplayTag or FGameplayAttribute
	 * It allows to omit requirement to create ValidateRecursive chains, where child most property can be validated only if all parents have ValidateRecursive
	 * Enabled by default
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	bool bAutoValidateStructInnerProperties = true;

	/**
	 * If set to true, will skip ALL blueprint generated classes, meaning all blueprints. Validation will start from first cpp base.
	 * Can slightly speed up validation if project doesn't use blueprint property validation that much, or uses deep blueprint hierarchies
	 * Disabled by default
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	bool bSkipBlueprintGeneratedClasses = false;

	/**
	 * If set to true, will automatically add "Validate" and "ValidateRecursive" meta specifiers when new blueprint component is added to a component tree
	 * Enabled by default
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	bool bAddMetaToNewBlueprintComponents = true;

	/**
	 * If set to true, will automatically add "Validate" and "ValidateRecursive" meta specifiers to newly created blueprint variables, if type supports
	 * Enabled by default
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	bool bAddMetaToNewBlueprintVariables = true;

	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	TSubclassOf<UObject> CustomizedObjectClass;

	/** List of classes with additional validation meta data */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	TArray<FClassExternalValidationData> ExternalClasses;

	/** List of structs with additional validation meta data */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	TArray<FStructExternalValidationData> ExternalStructs;
};
