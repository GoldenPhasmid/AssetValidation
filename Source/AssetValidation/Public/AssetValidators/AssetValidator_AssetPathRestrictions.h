#pragma once

#include "CoreMinimal.h"
#include "AssetValidators/AssetValidator.h"

#include "AssetValidator_AssetPathRestrictions.generated.h"

USTRUCT()
struct FAssetPathDescription
{
	GENERATED_BODY()

	bool PassesPathDescription(const FAssetData& AssetData) const;

	FORCEINLINE bool IsEmpty() const { return AssetDomains.IsEmpty() && RegexAssetPaths.IsEmpty(); }

	UPROPERTY(EditAnywhere, meta = (Validate, GetOptions=GetDomains))
	TArray<FString> AssetDomains;

	UPROPERTY(EditAnywhere, meta = (Validate))
	TArray<FString> RegexAssetPaths;
};

USTRUCT()
struct FAssetTypeDomainDescription
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (Validate, AllowAbstract = "true"))
	TArray<FSoftClassPath> AssetTypes;

	UPROPERTY(EditAnywhere)
	FAssetPathDescription AllowedPaths;

	UPROPERTY(EditAnywhere)
	FAssetPathDescription DisallowedPaths;
};

UCLASS(Config = Editor, DefaultConfig, DisplayName = "Asset Path Policy")
class ASSETVALIDATION_API UAssetPathPolicySettings: public UDeveloperSettings
{
	GENERATED_BODY()
public:

	static const UAssetPathPolicySettings& Get() { return *GetDefault<UAssetPathPolicySettings>(); }

	UPROPERTY(Config, EditAnywhere, Category = "Settings")
	FAssetPathDescription GlobalAllowedPaths;

	UPROPERTY(Config, EditAnywhere, Category = "Settings")
	FAssetPathDescription GlobalDisallowedPaths;

	UPROPERTY(Config, EditAnywhere, Category = "Settings")
	TArray<FAssetTypeDomainDescription> AssetRules;

	UFUNCTION()
	static TArray<FString> GetDomains();
};


UCLASS()
class UAssetValidator_AssetPathRestrictions: public UAssetValidator
{
	GENERATED_BODY()
public:
	UAssetValidator_AssetPathRestrictions();

	//~Begin AssetValidator
	virtual bool CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const override;
	virtual EDataValidationResult ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context) override;
	virtual EDataValidationResult ValidateAsset_Implementation(const FAssetData& InAssetData, FDataValidationContext& InContext) override;
	//~End AssetValidator interface

	static EDataValidationResult ValidateAssetPath(const FAssetPathDescription& AllowedPaths, const FAssetPathDescription& DisallowedPaths, const FAssetData& AssetData, FDataValidationContext& Context);
};
