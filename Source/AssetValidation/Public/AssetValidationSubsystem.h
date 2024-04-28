#pragma once

#include "CoreMinimal.h"
#include "EditorValidatorSubsystem.h"

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
	
	virtual EDataValidationResult IsStandaloneActorValid(AActor* Actor, TArray<FText>& ValidationErrors, TArray<FText>& ValidationWarnings, EDataValidationUsecase InValidationUsecase) const;
	//~Begin EditorValidatorSubsystem interface
	virtual EDataValidationResult IsAssetValid(const FAssetData& AssetData, TArray<FText>& ValidationErrors, TArray<FText>& ValidationWarnings, const EDataValidationUsecase InValidationUsecase) const override;
	virtual EDataValidationResult IsObjectValid(UObject* InObject, TArray<FText>& ValidationErrors, TArray<FText>& ValidationWarnings, const EDataValidationUsecase InValidationUsecase) const override;
	//~End EditorValidatorSubsystem interface

protected:

	/** @return true if asset not excluded from validation */
	bool CanValidateAsset(UObject* Asset) const;
	/** @return true if asset should be pre loaded for validation */
	bool ShouldLoadAsset(const FAssetData& AssetData) const;
	/** @return true if validator can be used for a given use case */
	bool CanUseValidator(const UEditorValidatorBase* Validator, EDataValidationUsecase Usecase) const;
	/** */
	EDataValidationResult IsActorValid(AActor* Actor, FDataValidationContext& Context) const;
};
