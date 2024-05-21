#include "AssetValidators/AssetValidator.h"

#include "Misc/DataValidation.h"
#include "AssetValidationModule.h"
#include "AssetValidationSettings.h"
#include "AssetRegistry/AssetDataToken.h"

EAssetValidationFlags Convert(EDataValidationUsecase Usecase)
{
	switch (Usecase)
	{
	case EDataValidationUsecase::None:			return EAssetValidationFlags::None;
	case EDataValidationUsecase::Manual:		return EAssetValidationFlags::Manual;
	case EDataValidationUsecase::Commandlet:	return EAssetValidationFlags::Commandlet;
	case EDataValidationUsecase::Save:			return EAssetValidationFlags::Save;
	case EDataValidationUsecase::PreSubmit:		return EAssetValidationFlags::PreSubmit;
	case EDataValidationUsecase::Script:		return EAssetValidationFlags::Script;
	default:
		checkNoEntry();
	}

	return EAssetValidationFlags::All;
}

UAssetValidator::UAssetValidator()
{
}

bool UAssetValidator::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	const bool bSuitableContext = !!(AllowedContext & Convert(InContext.GetValidationUsecase()));
	if (!bSuitableContext)
	{
		return false;
	}

	if (InObject == nullptr && !bAllowNullAsset)
	{
		// null assets are skipped
		return false;
	}

	FSoftClassPath ClassPath{InAssetData.GetClass()};
	return (AllowedClasses.IsEmpty() || AllowedClasses.Contains(ClassPath)) && !DisallowedClasses.Contains(ClassPath);
}

EDataValidationResult UAssetValidator::ValidateAsset(const FAssetData& InAssetData, FDataValidationContext& InContext)
{
	EDataValidationResult Result = EDataValidationResult::NotValidated;
	
	ResetValidationState();
	if (CanValidateAsset_Implementation(InAssetData, nullptr, InContext))
	{
		Result &= ValidateAsset_Implementation(InAssetData, InContext);
	}
	
	Result &= ExtractValidationState(InContext);
	return Result;
}

void UAssetValidator::LogValidatingAssetMessage(const FAssetData& AssetData, FDataValidationContext& Context)
{
	if (UAssetValidationSettings::Get()->bEnabledDetailedAssetLogging && AssetData.IsValid())
	{
		// can't use UEditorValidator::AssetMessage because AssetValidator resets its validation state when doing recursive validation
		Context.AddMessage(AssetData, EMessageSeverity::Info, NSLOCTEXT("AssetValidation", "ValidatingAsset", "Validating asset"));
	}
}
