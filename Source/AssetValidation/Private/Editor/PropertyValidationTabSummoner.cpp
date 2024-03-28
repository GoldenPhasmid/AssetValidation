#include "PropertyValidationTabSummoner.h"

#include "BlueprintEditor.h"

namespace UE::AssetValidation
{
	
FPropertyValidationTabSummoner::FPropertyValidationTabSummoner(TSharedPtr<FBlueprintEditor> InBlueprintEditor)
	: FWorkflowTabFactory("ValidationTab", InBlueprintEditor)
	, BlueprintEditor(InBlueprintEditor)
{
	bIsSingleton = true;

	TabLabel = NSLOCTEXT("AssetValidation", "ValidationTabLabel", "Validation");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon");
}

TSharedRef<SWidget> FPropertyValidationTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return FWorkflowTabFactory::CreateTabBody(Info);
}
	
}