#pragma once

#include "CoreMinimal.h"

#include "PropertyValidatorTests.generated.h"

UCLASS()
class UEmptyObject: public UObject
{
	GENERATED_BODY()
};

UCLASS()
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

UCLASS()
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

UCLASS()
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

UCLASS()
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
