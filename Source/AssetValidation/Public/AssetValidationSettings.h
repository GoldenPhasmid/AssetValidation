#pragma once

#include "CoreMinimal.h"
#include "AssetValidationSubsystem.h"
#include "Engine/DeveloperSettings.h"

#include "AssetValidationSettings.generated.h"

class UAVCommandletAction;
class UAVCommandletSearchFilter;

UCLASS(Config = Editor, DefaultConfig, DisplayName = "Asset Validation")
class ASSETVALIDATION_API UAssetValidationSettings: public UDeveloperSettings
{
	GENERATED_BODY()
public:

	UAssetValidationSettings(const FObjectInitializer& Initializer);

	static const UAssetValidationSettings* Get()
	{
		return GetDefault<UAssetValidationSettings>();
	}

	static UAssetValidationSettings* GetMutable()
	{
		return GetMutableDefault<UAssetValidationSettings>();
	}

	/** Default settings used for simple IsAssetValid/IsObjectValid validation requests */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	FValidateAssetsSettings DefaultSettings;

	UPROPERTY(EditAnywhere, Config, Category = "Settings", meta = (ContentDir))
	TArray<FDirectoryPath> ExcludedDirectories;

	/** If true, will fill validation log with messages like "Validating thingy" or "Done validating thingy" */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	bool bEnabledDetailedAssetLogging = false;

	/** If true, will drop package outer path from external actor hyperlink in validation log */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	bool bUseShortActorNames = true;
	
	/** If true, will open target actor's world if actor from validation log doesn't live in currently opened world @todo: implement  */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	bool bOpenEditorWorldForUnloadedActors = true;

	/** Default filter for @UAssetValidationCommandlet */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	TSubclassOf<UAVCommandletSearchFilter> CommandletDefaultFilter;

	/** Default action for @UAssetValidationCommandlet */
	UPROPERTY(EditAnywhere, Config, Category = "Settings")
	TSubclassOf<UAVCommandletAction> CommandletDefaultAction;
};
