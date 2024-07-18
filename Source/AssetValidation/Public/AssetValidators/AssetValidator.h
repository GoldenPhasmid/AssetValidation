#pragma once

#include "EditorValidatorBase.h"

#include "AssetValidator.generated.h"

UCLASS(Abstract, Blueprintable, Config = Editor, DefaultConfig, ConfigDoNotCheckDefaults)
class ASSETVALIDATION_API UAssetValidator: public UEditorValidatorBase
{
	GENERATED_BODY()
public:

	UAssetValidator();
	
	virtual void PostLoad() override;
	virtual void PostInitProperties() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;

	EDataValidationResult ValidateAsset(const FAssetData& InAssetData, FDataValidationContext& InContext);
	virtual EDataValidationResult ValidateAsset_Implementation(const FAssetData& InAssetData, FDataValidationContext& InContext)
	{
		return EDataValidationResult::NotValidated;
	}

protected:
	void LogValidatingAssetMessage(const FAssetData& AssetData, FDataValidationContext& Context);

	/**
	 * This property exists only because bOnlyPrintCustomMessage IS NOT CONFIG BUT EDITABLE FOR SOME REASON
	 * DataValidation is a horrible, I cannot extend anything without bashing my head into a wall of "clever implementation"
	 */
	UPROPERTY(Config)
	bool bLogCustomMessageOnly = false;
};
