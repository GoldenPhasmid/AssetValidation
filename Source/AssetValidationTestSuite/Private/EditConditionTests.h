#pragma once

#include "CoreMinimal.h"
#include "AutomationHelpers.h"

#include "EditConditionTests.generated.h"

UCLASS(HideDropdown)
class UValidationTestObject_EditCondition: public UObject
{
	GENERATED_BODY()
public:

	UPROPERTY()
	bool Bool = true;
	
	UPROPERTY()
	int32 Int = 32;

	UPROPERTY()
	ESimpleEnum Enum = ESimpleEnum::Two;
	
	UPROPERTY()
	UObject* Pointer = nullptr;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "Bool == false", Validate))
	UObject* BoolConditionFalse = nullptr;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "Bool == true", Validate))
	UObject* BoolConditionTrue = nullptr;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "Enum == ESimpleEnum::One", Validate))
	UObject* EnumConditionFalse = nullptr;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "Enum != ESimpleEnum::None", Validate))
	UObject* EnumConditionTrue = nullptr;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "Int == 8", Validate))
	UObject* IntConditionFalse = nullptr;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "Int >= 20", Validate))
	UObject* IntConditionTrue = nullptr;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "Pointer != nullptr", Validate))
	UObject* PointerConditionFalse = nullptr;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "Pointer == nullptr", Validate))
	UObject* PointerConditionTrue = nullptr;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "Pointer == nullptr && Bool == true && (Int < 10 || Enum == ESimpleEnum::None)", Validate))
	UObject* ComplexConditionFalse = nullptr;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "Pointer == nullptr || Bool == false || (Int == 32 && Enum == ESimpleEnum::Two)", Validate))
	UObject* ComplexConditionTrue = nullptr;
};
