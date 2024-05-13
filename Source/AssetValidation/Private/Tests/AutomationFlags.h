#pragma once

#include "AutomationFlags.generated.h"

namespace UE::AssetValidation
{
	static constexpr uint32 AutomationFlags = EAutomationTestFlags::ProductFilter | EAutomationTestFlags::ApplicationContextMask;
}

USTRUCT()
struct FStructWithoutValidator
{
	GENERATED_BODY()
};