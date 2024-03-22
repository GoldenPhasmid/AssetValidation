#pragma once

#include "CoreMinimal.h"

#include "PropertyValidationSettings.generated.h"

UCLASS(Config = Editor, DefaultConfig)
class ASSETVALIDATION_API UPropertyValidationSettings: public UDeveloperSettings
{
	GENERATED_BODY()
public:

	FORCEINLINE static const UPropertyValidationSettings* Get()
	{
		return GetDefault<UPropertyValidationSettings>();
	}

	static bool CanValidatePackage(const FString& PackageName);

	UPROPERTY(Config, meta = (Validate))
	TArray<FString> PackagesToValidate;
	
	UPROPERTY(Config)
	bool bSkipBlueprintGeneratedClasses = false;

	UPROPERTY(Config)
	bool bAutomaticallyValidateBlueprintComponents = true;

	UPROPERTY(Config)
	bool bAutomaticallyValidateBlueprintVariables = true;
};
