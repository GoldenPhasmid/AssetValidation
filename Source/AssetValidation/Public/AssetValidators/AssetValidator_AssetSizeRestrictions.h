#pragma once

#include "CoreMinimal.h"
#include "AssetValidators/AssetValidator.h"

#include "AssetValidator_AssetSizeRestrictions.generated.h"

/**
 * Size restriction for an asset type
 * You can specify AssetType restriction and then override it with Asset restriction
 */
USTRUCT()
struct FAssetTypeSizeRestriction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (Validate, AllowAbstract = "true"))
	FSoftClassPath ClassFilter;

	/** max asset type size with in-memory dependencies in MB */
	UPROPERTY(EditAnywhere)
	uint32 MaxMemorySizeMegaBytes = 0;

	/** max asset type size with disk dependencies in MB */
	UPROPERTY(EditAnywhere)
	uint32 MaxDiskSizeMegaBytes = 0;
};

/**
 * Size restriction for a particular asset
 * You can specify AssetType restriction and then override it with Asset restriction
 */
USTRUCT()
struct FAssetSizeRestriction
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, meta = (Validate, AllowAbstract = "true"))
	FSoftObjectPath Asset;

	/** max asset size with in-memory dependencies in MB */
	UPROPERTY(EditAnywhere)
	uint32 MaxMemorySizeMegaBytes = 0;

	/** max asset size with disk dependencies in MB */
	UPROPERTY(EditAnywhere)
	uint32 MaxDiskSizeMegaBytes = 0;
};

/**
 * Asset Size Restrictions
 * Verifies that asset size (both memory and disk) does not exceed max size in MB
 */
UCLASS(Config = Editor, DefaultConfig, DisplayName = "Asset Size Restrictions")
class ASSETVALIDATION_API UAssetValidator_AssetSizeRestrictions: public UAssetValidator
{
	GENERATED_BODY()
public:
	UAssetValidator_AssetSizeRestrictions();

	virtual void PostInitProperties() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	//~Begin AssetValidator interface
	virtual bool IsEnabled() const override;
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
	//~End AssetValidator interface

protected:
	
	/** */
	void BuildAssetTypeMap();
	/** */
	void GetAssetRestrictions(const UObject* Asset, float& OutMemory, float& OutDiskMemory) const;
	/** @return most derived native class for a given asset */
	UClass* GetCppAssetClass(const UObject* Asset) const;

	/** list of size restrictions per asset type */
	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation", meta = (Validate))
	TArray<FAssetTypeSizeRestriction> AssetTypes;

	/** list of size restrictions per asset */
	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation", meta = (Validate))
	TArray<FAssetSizeRestriction> Assets;

	/** A list of assets to skip during validation */
	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation", meta = (Validate))
	TArray<TSoftObjectPtr<UObject>> AssetExceptions;

	/** map between asset type and index into @AssetTypes array */
	UPROPERTY(Transient)
	TMap<FTopLevelAssetPath, int32> AssetTypeMap;

	/** map between asset and index into @AssetTypes array */
	UPROPERTY(Transient)
	TMap<FSoftObjectPath, int32> AssetMap;
	
};
