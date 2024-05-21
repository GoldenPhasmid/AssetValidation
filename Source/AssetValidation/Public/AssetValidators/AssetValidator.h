#pragma once

#include "EditorValidatorBase.h"

#include "AssetValidator.generated.h"

UENUM(BlueprintType)
enum class EAssetValidationFlags: uint8
{
	None		= 0		UMETA(Hidden),
	Manual		= 1,
	Commandlet	= 2,
	Save		= 4,
	PreSubmit	= 8,
	Script		= 16,
	// ...
	All			= 255	UMETA(Hidden)
};
ENUM_CLASS_FLAGS(EAssetValidationFlags);

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

	/** Contexts in which this validator can run */
	UPROPERTY(EditAnywhere, Config, meta = (BlueprintProtected = "true"))
	EAssetValidationFlags AllowedContext = EAssetValidationFlags::All;

	/** List of allowed classes. If empty, any class is allowed */
	UPROPERTY(EditAnywhere, Config, meta = (BlueprintProtected = "true"))
	TArray<FSoftClassPath> AllowedClasses;

	/** List of disallowed classes. If empty, any class is allowed */
	UPROPERTY(EditAnywhere, Config, meta = (BlueprintProtected = "true"))
	TArray<FSoftClassPath> DisallowedClasses;

	/** whether to allow validation for unloaded asset */
	UPROPERTY(EditAnywhere, Config, meta = (BlueprintProtected = "true"))
	bool bAllowNullAsset = false;
};
