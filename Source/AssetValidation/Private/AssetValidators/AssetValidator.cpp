#include "AssetValidators/AssetValidator.h"

#include "Misc/DataValidation.h"
#include "AssetValidationModule.h"
#include "AssetRegistry/AssetDataToken.h"

UAssetValidator::UAssetValidator()
{
}

bool UAssetValidator::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	return true;
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
	if (bLogValidatingAssetMessage == false || !AssetData.IsValid())
	{
		return;
	}
	// can't use AssetMessage because AssetValidator resets its validation state when doing recursive validation
	TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(EMessageSeverity::Info)
	->AddToken(FAssetDataToken::Create(AssetData))
	->AddToken(FTextToken::Create(NSLOCTEXT("AssetValidation", "ValidatingAsset", "Validating asset")));
	
	Context.AddMessage(Message);
}
