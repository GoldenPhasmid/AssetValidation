#pragma once

#include "AutomationHelpers.generated.h"

namespace UE::AssetValidation
{
	static constexpr uint32 AutomationFlags = EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask;
}

USTRUCT()
struct FStructWithoutValidator
{
	GENERATED_BODY()
};

UCLASS(HideDropdown)
class UEmptyObject: public UObject
{
	GENERATED_BODY()
};

UENUM()
enum class ESimpleEnum: uint8
{
	None = 0,
	One = 1,
	Two = 2
};
