#pragma once

#include "CoreMinimal.h"
#include "PropertyValidatorBase.h"

#include "StructValidator.generated.h"

UCLASS(Abstract)
class ASSETVALIDATION_API UStructValidator: public UPropertyValidatorBase
{
	GENERATED_BODY()
public:
	UStructValidator();

protected:

	/** @return script struct generated from a native non UHT compliant cpp struct */
	static UScriptStruct* GetNativeScriptStruct(FName StructName);
};

