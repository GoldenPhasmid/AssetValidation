#include "AssetValidationSubsystem.h"

#include "EditorValidatorBase.h"
#include "Misc/DataValidation.h"
#include "AssetValidationModule.h"
#include "AssetValidators/AssetValidator.h"
#include "Settings/ProjectPackagingSettings.h"

UAssetValidationSubsystem::UAssetValidationSubsystem()
{
	if (HasAllFlags(RF_ClassDefaultObject))
	{
		// this is a dirty hack to disable UEditorValidatorSubsystem creation and using this subsystem instead
		// I'm amazed that it actually works and doesn't break anything
		// there's no easy way in unreal to disable engine subsystems unless there's explicit support in ShouldCreateSubsystem
		UClass* Class = const_cast<UClass*>(UEditorValidatorSubsystem::StaticClass());
		Class->ClassFlags |= CLASS_Abstract;
	}
}

EDataValidationResult UAssetValidationSubsystem::IsStandaloneActorValid(AActor* Actor, TArray<FText>& ValidationErrors, TArray<FText>& ValidationWarnings, const EDataValidationUsecase InValidationUsecase) const
{
	EDataValidationResult Result = EDataValidationResult::NotValidated;
	if (Actor != nullptr)
	{
		FDataValidationContext Context;
		Result = IsActorValid(Actor, Context);
		Context.SplitIssues(ValidationWarnings, ValidationErrors);

		if (Result != EDataValidationResult::Invalid)
		{
			for (auto& ValidatorPair: Validators)
			{
				UEditorValidatorBase* Validator = ValidatorPair.Value;
				if (CanUseValidator(Validator, InValidationUsecase) && Validator->CanValidateAsset(Actor))
				{
					Validator->ResetValidationState();
					Result &= Validator->ValidateLoadedAsset(Actor, ValidationErrors);
					
					ValidationWarnings.Append(Validator->GetAllWarnings());
					ensureMsgf(Validator->IsValidationStateSet(), TEXT("Validator %s did not include a pass or fail state."), *Validator->GetClass()->GetName());
				}
			}
		}
	}

	return Result;
}

EDataValidationResult UAssetValidationSubsystem::IsAssetValid(const FAssetData& AssetData, TArray<FText>& ValidationErrors, TArray<FText>& ValidationWarnings, const EDataValidationUsecase InValidationUsecase) const
{
	EDataValidationResult Result = EDataValidationResult::Valid; // @todo: for some reason EDataValidationResult::NotValidated returns always NotValidated
	if (AssetData.FastGetAsset() != nullptr || ShouldLoadAsset(AssetData))
	{
		// call default implementation that loads an asset and calls IsObjectValid
		return Super::IsAssetValid(AssetData, ValidationErrors, ValidationWarnings, InValidationUsecase);
	}

	for (auto& ValidatorPair: Validators)
	{
		if (UAssetValidator* Validator = Cast<UAssetValidator>(ValidatorPair.Value))
		{
			if (CanUseValidator(Validator, InValidationUsecase))
			{
				Validator->ResetValidationState();
				// attempt to validate asset data. Asset validator may or may not load the asset in question
				Result &= Validator->ValidateAsset(AssetData, ValidationErrors);

				ValidationWarnings.Append(Validator->GetAllWarnings());
			}
		}
	}

	return Result;
}

EDataValidationResult UAssetValidationSubsystem::IsObjectValid(UObject* InObject, TArray<FText>& ValidationErrors, TArray<FText>& ValidationWarnings, const EDataValidationUsecase InValidationUsecase) const
{
	if (!CanValidateAsset(InObject))
	{
		// return that asset was not validated
		return EDataValidationResult::NotValidated;
	}
	
	return Super::IsObjectValid(InObject, ValidationErrors, ValidationWarnings, InValidationUsecase);
}

bool UAssetValidationSubsystem::CanValidateAsset(UObject* Asset) const
{
	if (Asset == nullptr)
	{
		return false;
	}
	
	const FString PackageName = Asset->GetPackage()->GetName();
	const UProjectPackagingSettings* PackagingSettings = GetDefault<UProjectPackagingSettings>();

	for (const FDirectoryPath& Directory: PackagingSettings->DirectoriesToNeverCook)
	{
		const FString& Folder = Directory.Path;
		if (PackageName.StartsWith(Folder))
		{
			return false;
		}
	}

	if (PackageName.StartsWith(TEXT("/Game/Developers/")))
	{
		return false;
	}

	return true;
}

bool UAssetValidationSubsystem::ShouldLoadAsset(const FAssetData& AssetData) const
{
	// don't load maps, map data or cooked packages
	return !AssetData.HasAnyPackageFlags(PKG_ContainsMap | PKG_ContainsMapData | PKG_Cooked);
}

bool UAssetValidationSubsystem::CanUseValidator(const UEditorValidatorBase* Validator, EDataValidationUsecase Usecase) const
{
	return Validator && Validator->IsEnabled() && Validator->CanValidate(Usecase);
}

EDataValidationResult UAssetValidationSubsystem::IsActorValid(AActor* Actor, FDataValidationContext& Context) const
{
	// This is a rough copy of a AActor::IsDataValid implementation, which has several reasons to exist:
	// - AActor::IsDataValid early outs on external actors (hello WP), which means actor components are skipped @todo verify
	// - AActor::IsDataValid calls CheckForErrors and if any warnings are encountered adds an error to validation context.
	// On the contrary, we don't want to call CheckForErrors, as map check is probably run somewhere in automation pipeline
	// or is expressed by another asset validator (that doesn't work with actors)
	// RF_HasExternalPackage flag allows us to early out in AActor::IsDataValid
	const bool bHasExternalPackage = Actor->HasAnyFlags(RF_HasExternalPackage);
	Actor->SetFlags(RF_HasExternalPackage);

	EDataValidationResult Result = EDataValidationResult::Valid;
	// run actual actor validation, skipping AActor::IsValid implementation
	Result &= Actor->IsDataValid(Context);

	// run default subobject validation
	if (CheckDefaultSubobjects() == false)
	{
		Result = EDataValidationResult::Invalid;
		const FText ErrorMsg = FText::Format(NSLOCTEXT("ErrorChecking", "IsDataValid_Failed_CheckDefaultSubobjectsInternal", "{0} failed CheckDefaultSubobjectsInternal()"), FText::FromString(GetName()));
		Context.AddError(ErrorMsg);
	}

	// validate actor components
	for (const UActorComponent* Component : Actor->GetComponents())
	{
		if (Component)
		{
			// if any component is invalid, our result is invalid
			// in the future we may want to update this to say that the actor was not validated if any of its components returns EDataValidationResult::NotValidated
			Result &= Component->IsDataValid(Context);
		}
	}

	if (!bHasExternalPackage)
	{
		// fix RF_HasExternalPackage flag
		Actor->ClearFlags(RF_HasExternalPackage);
	}

	return Result;
}
