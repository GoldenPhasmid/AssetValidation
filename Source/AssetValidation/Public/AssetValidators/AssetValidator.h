#pragma once

#include "EditorValidatorBase.h"

#include "AssetValidator.generated.h"

UCLASS(Abstract, Blueprintable)
class ASSETVALIDATION_API UAssetValidator: public UEditorValidatorBase
{
	GENERATED_BODY()
public:

	UAssetValidator();
	
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;

	EDataValidationResult ValidateAsset(const FAssetData& InAssetData, FDataValidationContext& InContext);
	virtual EDataValidationResult ValidateAsset_Implementation(const FAssetData& InAssetData, FDataValidationContext& InContext)
	{
		return EDataValidationResult::NotValidated;
	}

protected:

	void LogValidatingAssetMessage(const FAssetData& AssetData, FDataValidationContext& Context);

	UPROPERTY()
	bool bLogValidatingAssetMessage = false;
};
