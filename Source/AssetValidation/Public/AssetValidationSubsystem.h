#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorSubsystem.h"
#include "AssetValidationDefines.h"

#include "AssetValidationSubsystem.generated.h"

UCLASS()
class ASSETVALIDATION_API UAssetValidationSubsystem: public UEditorValidatorSubsystem
{
	GENERATED_BODY()
public:

	UAssetValidationSubsystem();

	FORCEINLINE static UAssetValidationSubsystem* Get()
	{
		return GEditor->GetEditorSubsystem<UAssetValidationSubsystem>();
	}
	
	//~Begin EditorValidatorSubsystem interface
#if !WITH_DATA_VALIDATION_UPDATE // world actor validation was fixed in 5.4
	virtual int32 ValidateAssetsWithSettings(const TArray<FAssetData>& AssetDataList, FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults) const override;
#endif
#if WITH_DATA_VALIDATION_UPDATE
	virtual int32 ValidateAssetsWithSettings(const TArray<FAssetData>& AssetDataList, FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults) const override;
#endif
	virtual EDataValidationResult IsAssetValidWithContext(const FAssetData& AssetData, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult IsObjectValidWithContext(UObject* InAsset, FDataValidationContext& InContext) const override;
	//~End EditorValidatorSubsystem interface

#if !WITH_DATA_VALIDATION_UPDATE // world actor validation was fixed in 5.4
	/** run asset validation on a standalone actor */
	virtual EDataValidationResult IsStandaloneActorValid(AActor* Actor, FDataValidationContext& Context) const;
#endif

protected:

	/** @return true if asset not excluded from validation */
	virtual bool ShouldValidateAsset(const FAssetData& Asset, const FValidateAssetsSettings& Settings, FDataValidationContext& InContext) const override;
	/** @return true if asset should be pre loaded for validation */
	bool ShouldLoadAsset(const FAssetData& AssetData) const;

	/** */
	void ResetValidationState() const;
	/** */
	mutable int32 CheckedAssetsCount = 0;
	/** */
	mutable TStaticArray<int32, 3> ValidationResults{InPlace, 0};
	
#if !WITH_DATA_VALIDATION_UPDATE // world actor validation was fixed in 5.4
	/** */
	EDataValidationResult IsActorValid(AActor* Actor, FDataValidationContext& Context) const;
#endif
};
