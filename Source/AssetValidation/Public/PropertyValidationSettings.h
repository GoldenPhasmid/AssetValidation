#pragma once

#include "CoreMinimal.h"

#include "PropertyValidationSettings.generated.h"

USTRUCT()
struct FEnginePropertyDescription
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TFieldPath<FProperty> PropertyPath;

	UPROPERTY(EditAnywhere)
	bool bValidate = false;

	UPROPERTY(EditAnywhere)
	bool bValidateKey = false;

	UPROPERTY(EditAnywhere)
	bool bValidateValue = false;

	UPROPERTY(EditAnywhere)
	bool bValidateRecursive = false;

	UPROPERTY(EditAnywhere)
	FString FailureMessage{};
};

USTRUCT()
struct FEngineClassDescription
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSoftClassPtr<UObject> EngineClass;

	UPROPERTY(EditAnywhere)
	TArray<FEnginePropertyDescription> Properties;
};

USTRUCT()
struct FEngineStructDescription
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UScriptStruct> StructClass;

	UPROPERTY(EditAnywhere)
	TArray<FEnginePropertyDescription> Properties;
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

	static bool CanValidatePackage(const FString& PackageName);

	UPROPERTY(EditAnywhere, Config, Category = "Settings", meta = (Validate))
	TArray<FString> PackagesToValidate;

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
	TFieldPath<FProperty> PropertyPath;

	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	TSubclassOf<UObject> ObjectClass;

	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	TSubclassOf<UObject> CustomizedObjectClass;

	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	TArray<FEngineClassDescription> EngineClasses;

	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	TArray<FEngineStructDescription> StructClasses;
};
