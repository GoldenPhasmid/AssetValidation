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
