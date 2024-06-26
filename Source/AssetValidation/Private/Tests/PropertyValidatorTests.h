#pragma once

#include "CoreMinimal.h"
#include "Tests/AutomationHelpers.h"

#include "PropertyValidatorTests.generated.h"

UCLASS(HideDropdown)
class UValidationTestObject_ValidationConditions: public UObject
{
	GENERATED_BODY()
public:
	
	UPROPERTY()
	UObject* NoMetaProperty;

	UPROPERTY(EditAnywhere)
	UObject* NoMetaEditableProperty;

	/** property not editable in editor, no validation */
	UPROPERTY(meta = (Validate))
	UObject* NonEditableProperty;

	/** property is transient, no validation */
	UPROPERTY(Transient, meta = (Validate))
	UObject* TransientProperty;

	/** property is transient, no validation */
	UPROPERTY(EditAnywhere, Transient, meta = (Validate))
	UObject* TransientEditableProperty;

	/** property is editable, validated */
	UPROPERTY(EditDefaultsOnly, meta = (Validate))
	UObject* EditDefaultOnlyProperty;

	/** property is editable, validated */
	UPROPERTY(EditAnywhere, meta = (Validate))
	UObject* EditAnywhereProperty;

	/** property is editable, validated */
	UPROPERTY(EditInstanceOnly, meta = (Validate))
	UObject* EditInstanceOnlyProperty;
	
};

UCLASS(HideDropdown)
class UValidationTestObject_PropertyTypes: public UObject
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, meta = (Validate))
	TObjectPtr<UObject> ObjectProperty;

	UPROPERTY(EditAnywhere, meta = (Validate))
	TSubclassOf<UObject> ClassProperty;

	UPROPERTY(EditAnywhere, meta = (Validate))
	TSoftObjectPtr<UObject> SoftObjectProperty;

	UPROPERTY(EditAnywhere, meta = (Validate))
	TSoftClassPtr<UClass> SoftClassProperty;

	UPROPERTY(EditAnywhere, meta = (Validate))
	FName Name;
	
	UPROPERTY(EditAnywhere, meta = (Validate))
	FString Str;
	
	UPROPERTY(EditAnywhere, meta = (Validate))
	FText Text;

	UPROPERTY(EditAnywhere, meta = (Validate))
	ESimpleEnum Enum = ESimpleEnum::None;
};


UCLASS(HideDropdown)
class UNestedObject: public UObject
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, meta = (Validate))
	UObject* InvalidProperty1 = nullptr;

	UPROPERTY(EditAnywhere, meta = (Validate))
	UObject* InvalidProperty2 = nullptr;

	UPROPERTY(EditAnywhere, meta = (Validate))
	UObject* InvalidProperty3 = nullptr;

	UPROPERTY(EditAnywhere, meta = (Validate))
	FString ValidProperty = "0123456789";
};

UCLASS(HideDropdown)
class UValidationTestObject_ValidationMetas: public UObject
{
	GENERATED_BODY()
public:
	
	UPROPERTY(EditAnywhere, meta = (Validate))
	UObject* Validate;

	UPROPERTY(EditAnywhere, meta = (ValidateRecursive))
	UObject* ValidateRecursive;

	UPROPERTY(EditAnywhere, meta = (ValidateRecursive))
	UObject* ValidateBoth;
	
	UPROPERTY(EditAnywhere, meta = (Validate, FailureMessage="my custom message"))
	UObject* ValidateWithCustomMessage;

	UPROPERTY(EditAnywhere, meta = (ValidateKey))
	TMap<UObject*, UObject*> ValidateKey;

	UPROPERTY(EditAnywhere, meta = (ValidateValue))
	TMap<UObject*, UObject*> ValidateValue;
};

UCLASS(HideDropdown)
class UValidationTestObject_ContainerProperties: public UObject
{
	GENERATED_BODY()
public:
	
	UPROPERTY(EditAnywhere, meta = (Validate))
	TArray<UObject*> ObjectArray;

	UPROPERTY(EditAnywhere, meta = (Validate))
	TSet<UObject*> ObjectSet;

	UPROPERTY(EditAnywhere, meta = (Validate))
	TMap<UObject*, UObject*> ObjectMap;
};

UCLASS(HideDropdown)
class UValidationTestDataAsset: public UDataAsset
{
	GENERATED_BODY()
public:
	
	UPROPERTY()
	UObject* NoMetaProperty;

	UPROPERTY(EditAnywhere)
	UObject* NoMetaEditableProperty;

	/** property not editable in editor, no validation */
	UPROPERTY(meta = (Validate))
	UObject* NonEditableProperty;

	/** property is transient, no validation */
	UPROPERTY(Transient, meta = (Validate))
	UObject* TransientProperty;

	/** property is transient, no validation */
	UPROPERTY(EditAnywhere, Transient, meta = (Validate))
	UObject* TransientEditableProperty;

	/** property is editable, validated */
	UPROPERTY(EditDefaultsOnly, meta = (Validate))
	UObject* EditDefaultOnlyProperty;

	/** property is editable, validated */
	UPROPERTY(EditAnywhere, meta = (Validate))
	UObject* EditAnywhereProperty;

	/** property is editable, validated */
	UPROPERTY(EditInstanceOnly, meta = (Validate))
	UObject* EditInstanceOnlyProperty;
};