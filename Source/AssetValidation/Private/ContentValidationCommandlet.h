#pragma once

#include "Commandlets/Commandlet.h"

#include "ContentValidationCommandlet.generated.h"

UCLASS()
class UContentValidationCommandlet: public UCommandlet
{
	GENERATED_BODY()
public:

	virtual int32 Main(const FString& Commandline) override;
	
};
