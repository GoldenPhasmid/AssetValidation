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

	void DisableValidators(TConstArrayView<FString> ClassNames);
	TArray<FName> GetPackagePaths(const TMap<FString, FString>& Params, TConstArrayView<FString> Switches) const;
	TArray<FTopLevelAssetPath> GetAllowedClasses(const TMap<FString, FString>& Params, TConstArrayView<FString> Switches) const;
	
	FValidateAssetsResults ValidateAssets(const TArray<FAssetData>& Assets, FValidateAssetsSettings& Settings) const;
};
