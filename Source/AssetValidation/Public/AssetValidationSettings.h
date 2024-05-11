#pragma once

#include "CoreMinimal.h"
#include "AssetValidationSubsystem.h"
#include "Engine/DeveloperSettings.h"

#include "AssetValidationSettings.generated.h"

UCLASS(Config = Editor, DefaultConfig)
class ASSETVALIDATION_API UAssetValidationSettings: public UDeveloperSettings
{
	GENERATED_BODY()
public:

	static const UAssetValidationSettings* Get()
	{
		return GetDefault<UAssetValidationSettings>();
	}

	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	FValidateAssetsSettings DefaultSettings;

	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	bool bEnabledDetailedAssetLogging = false;
	
};
