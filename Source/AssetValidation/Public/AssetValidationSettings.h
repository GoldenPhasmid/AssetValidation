#pragma once

#include "CoreMinimal.h"
#include "AssetValidationSubsystem.h"
#include "Engine/DeveloperSettings.h"

#include "AssetValidationSettings.generated.h"

UCLASS(Config = Editor, DefaultConfig, DisplayName = "Asset Validation")
class ASSETVALIDATION_API UAssetValidationSettings: public UDeveloperSettings
{
	GENERATED_BODY()
public:

	static const UAssetValidationSettings* Get()
	{
		return GetDefault<UAssetValidationSettings>();
	}

	/** Default settings used for simple IsAssetValid/IsObjectValid validation requests */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	FValidateAssetsSettings DefaultSettings;

	/**
	 * If set to true, packages will be reloaded to check for loading errors
	 * @todo replace with explicit UAssetValidator_LoadPackage options
	 */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	bool bReloadPackages = true;

	/** If true, will fill validation log with messages like "Validating thingy" or "Done validating thingy" */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	bool bEnabledDetailedAssetLogging = false;

	/** If true, will drop package outer path from external actor hyperlink in validation log */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	bool bUseShortActorNames = true;
	
	/** If true, will open target actor's world if actor from validation log doesn't live in currently opened world @todo: implement  */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	bool bOpenEditorWorldForUnloadedActors = true;
};
