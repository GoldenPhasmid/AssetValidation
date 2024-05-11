#include "ValidationTabSummoner.h"

#include "BlueprintEditor.h"
#include "UserDefinedStructValidationTabLayout.h"

namespace UE::AssetValidation
{
	
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

void SUserDefinedStructValidationTab::Construct(const FArguments& Args)
{
	UserDefinedStruct = Args._Struct;

	StructScope = MakeShared<FStructOnScope>(UserDefinedStruct.Get());
	UserDefinedStruct->InitializeDefaultValue(StructScope->GetStructMemory());
	StructScope->SetPackage(UserDefinedStruct->GetOutermost());

	FPropertyEditorModule& PropertyEditor = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.bShowOptions = false;
	
	DetailsView = PropertyEditor.CreateDetailView(DetailsViewArgs);
	DetailsView->RegisterInstancedCustomPropertyLayout(UUserDefinedStruct::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FUserDefinedStructValidationTabLayout::MakeInstance, StructScope));
	DetailsView->SetObject(UserDefinedStruct.Get());
	
	ChildSlot
	[
		DetailsView.ToSharedRef()
	];
}

	void SUserDefinedStructValidationTab::PreChange(const UUserDefinedStruct* Struct, FStructureEditorUtils::EStructureEditorChangeInfo Info)
{
	if (Info != FStructureEditorUtils::DefaultValueChanged)
	{
		StructScope->Destroy();
		DetailsView->SetObject(nullptr);
	}
}

void SUserDefinedStructValidationTab::PostChange(const UUserDefinedStruct* Struct, FStructureEditorUtils::EStructureEditorChangeInfo Info)
{
	if (Info != FStructureEditorUtils::DefaultValueChanged)
	{
		StructScope->Initialize(UserDefinedStruct.Get());
		// Force the set object call because we may be called multiple times in a row if more than one struct was changed at the same time
		DetailsView->SetObject(UserDefinedStruct.Get(), true);
	}

	UserDefinedStruct.Get()->InitializeDefaultValue(StructScope->GetStructMemory());
}

	
}
