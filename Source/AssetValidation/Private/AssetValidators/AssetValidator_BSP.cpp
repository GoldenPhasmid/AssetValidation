#include "AssetValidators/AssetValidator_BSP.h"

#include "ActorEditorUtils.h"
#include "AssetValidationDefines.h"
#include "Components/ModelComponent.h"
#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

UAssetValidator_BSP::UAssetValidator_BSP()
{
	LevelHasBSPSubmitFailedText = LOCTEXT(
				"AssetValidator_LevelHasBSP",
				"{0} contains {1} BSP geometry brushes and is not allowed to submit to source control. "
				"Please convert BSP brushes to static meshes before submit.");
	BSPBrushSubmitFailedText = LOCTEXT(
			"AssetValidator_BSPBrush",
			"{0} represents BSP brush which are not allowed to submit to source control. "
			"Please convert BSP brushes to static meshes before submit.");
	bIsConfigDisabled = true; // disabled by default

	bCanRunParallelMode = true;
	bRequiresLoadedAsset = true;
	bRequiresTopLevelAsset = false;
	bCanValidateActors = true;
}

bool UAssetValidator_BSP::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	if (!Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext))
	{
		return false;
	}
	
	if (InContext.GetValidationUsecase() != EDataValidationUsecase::PreSubmit)
	{
		return false;
	}

	const bool bCorrectClass = InObject != nullptr && (InObject->IsA<ULevel>() || (InObject->IsA<ABrush>() && !InObject->IsA<AVolume>()));
	if (bCorrectClass == false)
	{
		return false;
	}

	FSoftObjectPath OuterAssetPath{InAssetData.GetSoftObjectPath().GetAssetPath(), {}};
	if (OuterAssetPath.IsNull() || (WorldFilter.Num() > 0 && !WorldFilter.Contains(OuterAssetPath)))
	{
		return false;
	}

	return true;
}

EDataValidationResult UAssetValidator_BSP::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& InContext)
{
	check(InAsset);
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_BSP, AssetValidationChannel);

	if (ULevel* Level = Cast<ULevel>(InAsset))
	{
		if (int32 ModelCount = Level->ModelComponents.Num(); ModelCount > 0)
		{
			const FText FailReason = FText::Format(
				LevelHasBSPSubmitFailedText,
				FText::FromString(Level->GetWorld()->GetName()),
				FText::FromString(FString::FromInt(ModelCount))
			);
			InContext.AddMessage(InAssetData, EMessageSeverity::Error, FailReason);
			return EDataValidationResult::Invalid;
		}
	}
	if (ABrush* Brush = Cast<ABrush>(InAsset); Brush && !FActorEditorUtils::IsABuilderBrush(Brush))
	{
		const FText FailReason = FText::Format(BSPBrushSubmitFailedText, FText::FromName(InAssetData.AssetName)
		);
		InContext.AddMessage(InAssetData, EMessageSeverity::Error, FailReason);
	}
	
	return EDataValidationResult::Valid;
}

#undef LOCTEXT_NAMESPACE