#pragma once

#include "EditorValidatorBase.h"

#include "AssetValidator.generated.h"

UCLASS(Abstract, Blueprintable, Config = Editor, DefaultConfig, ConfigDoNotCheckDefaults)
class ASSETVALIDATION_API UAssetValidator: public UEditorValidatorBase
{
	GENERATED_BODY()
public:
	UAssetValidator();

	//~Begin UObject interface
	virtual void PostLoad() override;
	virtual void PostInitProperties() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~End UObject interface
	
	FORCEINLINE bool CanRunParallelMode() const { return bCanRunParallelMode; }
	FORCEINLINE bool RequiresLoadedAsset() const { return bRequiresLoadedAsset; }
	FORCEINLINE bool CanValidateActors() const { return bCanValidateActors; }
	
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;

	EDataValidationResult ValidateAsset(const FAssetData& InAssetData, FDataValidationContext& InContext);
	virtual EDataValidationResult ValidateAsset_Implementation(const FAssetData& InAssetData, FDataValidationContext& InContext)
	{
		return EDataValidationResult::NotValidated;
	}

	FORCEINLINE void SetEnabled(bool bNewEnabled)
	{
		bIsEnabled = bNewEnabled;
	}

protected:
	void LogValidatingAssetMessage(const FAssetData& AssetData, FDataValidationContext& Context);

	/**
	 * This property exists only because bOnlyPrintCustomMessage IS NOT CONFIG BUT EDITABLE FOR SOME REASON
	 * DataValidation is a horrible, I cannot extend anything without bashing my head into a wall of "clever implementation"
	 */
	UPROPERTY(Config)
	uint8 bLogCustomMessageOnly : 1 = false;
	
	/**
	 * Indicates whether validator can run in parallel with other validators with the same trait.
	 * It usually means that validator uses read-only operations and does not modify any state
	 */
	uint8 bCanRunParallelMode: 1 = false;
	/** Indicates whether asset should be pre-loaded from asset data before calling CanValidateAsset */
	uint8 bRequiresLoadedAsset: 1 = true;
	/** Indicates whether validator can operate only on top level assets (assets in Content Browser) */
	uint8 bRequiresTopLevelAsset: 1 = true;
	/** Indicates whether validator is an actor validator as well */
	uint8 bCanValidateActors : 1 = false;
};
