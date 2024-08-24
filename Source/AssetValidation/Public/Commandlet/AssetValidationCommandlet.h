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

protected:
	void FindAssets(const TArray<FString>& Switches, const TMap<FString, FString>& Params, TArray<FAssetData>& OutAssets);
	
	TArray<FName> GetPackagePaths(const TMap<FString, FString>& Params, TConstArrayView<FString> Switches) const;
	TArray<FTopLevelAssetPath> GetAllowedClasses(const TMap<FString, FString>& Params, TConstArrayView<FString> Switches) const;
};
