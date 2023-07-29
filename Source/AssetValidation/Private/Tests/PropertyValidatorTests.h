#pragma once

#include "CoreMinimal.h"

#include "PropertyValidatorTests.generated.h"

UCLASS()
class UTestObject_ValidationConditions: public UObject
{
	GENERATED_BODY()
public:

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