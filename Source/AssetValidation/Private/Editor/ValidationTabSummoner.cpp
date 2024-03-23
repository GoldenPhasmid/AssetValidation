#include "ValidationTabSummoner.h"

#include "BlueprintEditor.h"

FValidationTabSummoner::FValidationTabSummoner(TSharedPtr<FBlueprintEditor> InBlueprintEditor)
	: FWorkflowTabFactory("ValidationTab", InBlueprintEditor)
	, BlueprintEditor(InBlueprintEditor)
{
	bIsSingleton = true;

	TabLabel = NSLOCTEXT("AssetValidation", "ValidationTabLabel", "Validation");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon");
}

TSharedRef<SWidget> FValidationTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return FWorkflowTabFactory::CreateTabBody(Info);
}
