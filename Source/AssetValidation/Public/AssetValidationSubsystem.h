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

	static void MarkPackageLoaded(const FName& PackageName);
	static bool IsPackageAlreadyLoaded(const FName& PackageName);
	
	//~Begin EditorValidatorSubsystem interface
#if !WITH_DATA_VALIDATION_UPDATE // world actor validation was fixed in 5.4
	virtual int32 ValidateAssetsWithSettings(const TArray<FAssetData>& AssetDataList, FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults) const override;
#else
	virtual int32 ValidateAssetsWithSettings(const TArray<FAssetData>& AssetDataList, FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults) const override;
#endif
	
	virtual EDataValidationResult IsAssetValidWithContext(const FAssetData& AssetData, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult IsObjectValidWithContext(UObject* InAsset, FDataValidationContext& InContext) const override;
	
#if WITH_DATA_VALIDATION_UPDATE
	virtual EDataValidationResult ValidateChangelist(UDataValidationChangelist* InChangelist, const FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults) const override;
	virtual EDataValidationResult ValidateChangelists(const TArray<UDataValidationChangelist*> InChangelists, const FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults) const override;
	virtual void GatherAssetsToValidateFromChangelist(UDataValidationChangelist* InChangelist, const FValidateAssetsSettings& Settings, TSet<FAssetData>& OutAssets, FDataValidationContext& InContext) const override;
#endif
	//~End EditorValidatorSubsystem interface

#if !WITH_DATA_VALIDATION_UPDATE // world actor validation was fixed in 5.4
	/** run asset validation on a standalone actor */
	virtual EDataValidationResult IsStandaloneActorValid(AActor* Actor, FDataValidationContext& Context) const;
#endif

protected:
	
	bool IsEmptyChangelist(UDataValidationChangelist* Changelist) const;

	void ValidateAssetsResolverInternal(
		FMessageLog& 					DataValidationLog,
		const TArray<FAssetData>&		AssetDataList,
		const FValidateAssetsSettings& 	InSettings,
		FValidateAssetsResults& 		OutResults
	) const;
	
	void ValidateAssetsInternal(
		FMessageLog& 					DataValidationLog,
		TArray<FAssetData>				AssetDataList,
		const FValidateAssetsSettings& 	InSettings,
		FValidateAssetsResults& 		OutResults
	) const;
	
	/** @return true if asset not excluded from validation */
	virtual bool ShouldValidateAsset(const FAssetData& Asset, const FValidateAssetsSettings& Settings, FDataValidationContext& InContext) const override;
	/** @return true if asset should be pre loaded for validation */
	bool ShouldLoadAsset(const FAssetData& AssetData) const;

	UPROPERTY(Config)
	bool bLogEveryAssetBecauseYouWantYourLogThrashed = false;

	/** */
	void ResetValidationState() const;
	void ResetValidationState();
	/** */
	mutable int32 CheckedAssetsCount = 0;
	/** */
	mutable TStaticArray<int32, 3> ValidationResults{InPlace, 0};
	/** guard for recursive validation requests to this subsystem */
	mutable bool bRecursiveCall = false;
	/** Settings for running validation request */
	mutable TOptional<FValidateAssetsSettings> CurrentSettings;
	/** Packages that are loaded as a part of running validation request */
	TSet<FName> LoadedPackageNames;
	
#if !WITH_DATA_VALIDATION_UPDATE // world actor validation was fixed in 5.4
	/** */
	EDataValidationResult IsActorValid(AActor* Actor, FDataValidationContext& Context) const;
#endif
};
