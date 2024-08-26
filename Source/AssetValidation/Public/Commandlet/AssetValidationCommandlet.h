#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"

#include "AssetValidationCommandlet.generated.h"

struct FValidateAssetsSettings;
struct FValidateAssetsResults;

UCLASS()
class UAssetValidationCommandlet: public UCommandlet
{
	GENERATED_BODY()
public:

	virtual int32 Main(const FString& Commandline) override;

	static void ParseCommandlineParams(UObject* Target, const TArray<FString>& Switches, const TMap<FString, FString>& Params);
};
