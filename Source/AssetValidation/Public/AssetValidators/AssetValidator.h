#pragma once

#include "EditorValidatorBase.h"

#include "AssetValidator.generated.h"

UENUM(BlueprintType)
enum EAssetValidationFlags: int32
{
	None		= 0x00000000	UMETA(Hidden),
	Manual		= 0x00000001,
	Commandlet	= 0x00000002,
	Save		= 0x00000004,
	PreSubmit	= 0x00000008,
	Script		= 0x00000010,
	// ...
	All			= 0x7FFFFFFF	UMETA(Hidden)
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
	UPROPERTY(EditAnywhere, Config, meta = (BlueprintProtected = "true", Bitmask, BitmaskEnum = "/Script/AssetValidation.EAssetValidationFlags"))
	int32 AllowedContext = 0;

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
