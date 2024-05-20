#pragma once

#include "CoreMinimal.h"

#include "EditConditionTests.generated.h"

UENUM()
enum class ETestEnum: uint8
{
	None,
	One,
	Two,
};

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
	ETestEnum Enum = ETestEnum::Two;
	
	UPROPERTY()
	UObject* Pointer = nullptr;

	UPROPERTY(meta = (EditCondition = "Bool == false"))
	UObject* BoolConditionFalse = nullptr;

	UPROPERTY(meta = (EditCondition = "Bool == true"))
	UObject* BoolConditionTrue = nullptr;

	UPROPERTY(meta = (EditCondition = "Enum == ETestEnum::One"))
	UObject* EnumConditionFalse = nullptr;

	UPROPERTY(meta = (EditCondition = "Enum != ETestEnum::None"))
	UObject* EnumConditionTrue = nullptr;

	UPROPERTY(meta = (EditCondition = "Int == 8"))
	UObject* IntConditionFalse = nullptr;

	UPROPERTY(meta = (EditCondition = "Int >= 20"))
	UObject* IntConditionTrue = nullptr;

	UPROPERTY(meta = (EditCondition = "Pointer != nullptr"))
	UObject* PointerConditionFalse = nullptr;

	UPROPERTY(meta = (EditCondition = "Pointer == nullptr"))
	UObject* PointerConditionTrue = nullptr;

	UPROPERTY(meta = (EditCondition = "Pointer == nullptr && Bool == true && (Int < 10 || Enum == ETestEnum::None)"))
	UObject* ComplexConditionFalse = nullptr;

	UPROPERTY(meta = (EditCondition = "Pointer == nullptr || Bool == false || (Int == 32 && Enum == ETestEnum::Two)"))
	UObject* ComplexConditionTrue = nullptr;
};
