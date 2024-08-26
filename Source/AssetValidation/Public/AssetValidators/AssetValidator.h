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

	//~Begin AssetValidator interface
	FORCEINLINE bool IsThreadSafe() const { return bThreadSafe; }
	FORCEINLINE bool RequiresLoadedAsset() const { return bRequiresLoadedAsset; }
	FORCEINLINE bool CanValidateActors() const { return bCanValidateActors; }
	//~End AssetValidator interface
	
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

	// Validator traits
	/** */
	uint8 bThreadSafe: 1;
	/** */
	uint8 bRequiresLoadedAsset: 1;
	/** */
	uint8 bCanValidateActors : 1;
};
