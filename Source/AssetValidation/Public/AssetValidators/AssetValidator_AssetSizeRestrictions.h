#pragma once

#include "CoreMinimal.h"
#include "AssetValidators/AssetValidator.h"

#include "AssetValidator_AssetSizeRestrictions.generated.h"

USTRUCT()
struct FAssetTypeSizeRestriction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (Validate, AllowAbstract = "true"))
	FSoftClassPath ClassFilter;

	UPROPERTY(EditAnywhere)
	uint32 MaxMemorySizeMegaBytes = 0;
	
	UPROPERTY(EditAnywhere)
	uint32 MaxDiskSizeMegaBytes = 0;
};

USTRUCT()
struct FAssetSizeRestriction
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, meta = (Validate, AllowAbstract = "true"))
	FSoftObjectPath Asset;

	UPROPERTY(EditAnywhere)
	uint32 MaxMemorySizeMegaBytes = 0;
	
	UPROPERTY(EditAnywhere)
	uint32 MaxDiskSizeMegaBytes = 0;
};

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
	/** */
	UClass* GetCppAssetClass(const UObject* Asset) const;
	
	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation", meta = (Validate))
	TArray<FAssetTypeSizeRestriction> AssetTypes;

	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation", meta = (Validate))
	TArray<FAssetSizeRestriction> Assets;
	
	UPROPERTY(Config, EditAnywhere, Category = "Asset Validation", meta = (Validate))
	TArray<TSoftObjectPtr<UObject>> AssetExceptions;

	UPROPERTY(Transient)
	TMap<FTopLevelAssetPath, int32> AssetTypeMap;
	
	UPROPERTY(Transient)
	TMap<FSoftObjectPath, int32> AssetMap;
	
};
