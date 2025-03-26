#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorSubsystem.h"

#include "AssetValidationSubsystem.generated.h"

class UAssetValidator;

UCLASS()
class ASSETVALIDATION_API UAssetValidationSubsystem: public UEditorValidatorSubsystem
{
	GENERATED_BODY()
public:

	UAssetValidationSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	FORCEINLINE static UAssetValidationSubsystem* Get()
	{
		return GEditor->GetEditorSubsystem<UAssetValidationSubsystem>();
	}

	static void MarkPackageLoaded(const FName& PackageName);
	static bool IsPackageAlreadyLoaded(const FName& PackageName);
	
	//~Begin EditorValidatorSubsystem interface
	virtual int32 ValidateAssetsWithSettings(const TArray<FAssetData>& AssetDataList, FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults) const override;
	virtual EDataValidationResult IsAssetValidWithContext(const FAssetData& AssetData, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult IsObjectValidWithContext(UObject* InAsset, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateChangelist(UDataValidationChangelist* InChangelist, const FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults) const override;
	virtual EDataValidationResult ValidateChangelists(const TArray<UDataValidationChangelist*> InChangelists, const FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults) const override;
	virtual void GatherAssetsToValidateFromChangelist(UDataValidationChangelist* InChangelist, const FValidateAssetsSettings& Settings, TSet<FAssetData>& OutAssets, FDataValidationContext& InContext) const override;
	//~End EditorValidatorSubsystem interface

	EDataValidationResult IsActorValidWithContext(const FAssetData& AssetData, AActor* Actor, FDataValidationContext& InContext) const;

	/** @return editor validator of a requested type */
	UEditorValidatorBase* GetValidator(TSubclassOf<UAssetValidator> ValidatorClass) const;
	
	/** @return editor validator of a requested type */
	template <typename TValidatorType> requires std::is_base_of_v<UEditorValidatorBase, TValidatorType>
	TValidatorType* GetValidator() const
	{
		return CastChecked<TValidatorType>(GetValidator(TValidatorType::StaticClass()));
	}
protected:
	
	bool ShouldShowCancelButton(int32 NumAssets, const FValidateAssetsSettings& InSettings) const;
	bool IsEmptyChangelist(UDataValidationChangelist* Changelist) const;

	EDataValidationResult ValidateAssetsInternalResolver(
		FMessageLog& 					DataValidationLog,
		const TArray<FAssetData>&		AssetDataList,
		const FValidateAssetsSettings& 	InSettings,
		FValidateAssetsResults& 		OutResults
	) const;
	
	EDataValidationResult ValidateAssetsInternal(
		FMessageLog& 					DataValidationLog,
		TArray<FAssetData>				AssetDataList,
		const FValidateAssetsSettings& 	InSettings,
		FValidateAssetsResults& 		OutResults
	) const;

	EDataValidationResult ValidateChangelistsInternal(
	FMessageLog& 								DataValidationLog,
	TConstArrayView<UDataValidationChangelist*> Changelists,
	const FValidateAssetsSettings& 				Settings,
	FValidateAssetsResults& 					OutResults) const;

	
	/** @return true if asset not excluded from validation */
	virtual bool ShouldValidateAsset(const FAssetData& Asset, const FValidateAssetsSettings& Settings, FDataValidationContext& InContext) const override;
	/** @return true if asset should be pre loaded for validation */
	bool ShouldLoadAsset(const FAssetData& AssetData) const;

	void MarkAssetDataValidated(const FAssetData& AssetData, EDataValidationResult Result) const;
	
	/** */
	void ResetValidationState() const;
	void ResetValidationState();

	UPROPERTY(Transient)
	TArray<UAssetValidator*> ActorValidators;
	
	/** number of assets checked, stored as a part of running asset validation */
	mutable int32 CheckedAssetsCount = 0;
	/** asset validation results, stored as a part of running asset validation */
	mutable TStaticArray<int32, 3> ValidationResults{InPlace, 0};
	/** guard for recursive validation requests to this subsystem */
	mutable bool bRecursiveCall = false;
	/** Settings for running validation request */
	mutable TOptional<FValidateAssetsSettings> CurrentSettings;
	/** Packages that are loaded as a part of a running validation request */
	TSet<FName> LoadedPackageNames;
	/** Assets validated as a part of a running validation request */
	mutable TSet<FAssetData> ValidatedAssets;
};
