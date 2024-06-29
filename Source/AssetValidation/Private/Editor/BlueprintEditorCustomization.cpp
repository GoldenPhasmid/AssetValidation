#include "BlueprintEditorCustomization.h"

#include "BlueprintComponentCustomization.h"
#include "BlueprintEditor.h"
#include "BlueprintVariableCustomization.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "PropertyValidationDetailsBuilder.h"
#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

FValidationTabSummoner::FValidationTabSummoner(TSharedPtr<FBlueprintEditor> InBlueprintEditor)
	: FWorkflowTabFactory("ValidationTab", InBlueprintEditor)
	  , BlueprintEditor(InBlueprintEditor)
{
	bIsSingleton = true;

	TabLabel = LOCTEXT("ValidationTabTitle", "Validation");
	TabIcon = FSlateIcon(FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon");
}

TSharedRef<SWidget> FValidationTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(SBlueprintEditorValidationTab).BlueprintEditor(BlueprintEditor);
}

void SBlueprintEditorValidationTab::Construct(const FArguments& Args)
{
	BlueprintEditor = Args._BlueprintEditor;

	FPropertyEditorModule& PropertyEditor = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.NotifyHook = this;

	DetailsView = PropertyEditor.CreateDetailView(DetailsViewArgs);
	auto CustomLayoutDelegate = FOnGetDetailCustomizationInstance::CreateStatic(&FBlueprintEditorValidationTabLayout::MakeInstance, BlueprintEditor);
	DetailsView->RegisterInstancedCustomPropertyLayout(UBlueprintValidationView::StaticClass(), CustomLayoutDelegate);

	BlueprintView = NewObject<UBlueprintValidationView>();
	if (UBlueprint* Blueprint = BlueprintEditor.Pin()->GetBlueprintObj())
	{
		Blueprint->OnChanged().AddSP(this, &SBlueprintEditorValidationTab::OnBlueprintChanged);
	}

	UpdateBlueprintView();
	ChildSlot
	[
		DetailsView.ToSharedRef()
	];
}

void SBlueprintEditorValidationTab::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(BlueprintView);
}

void SBlueprintEditorValidationTab::NotifyPreChange(FProperty* PropertyAboutToChange)
{
	DetailsView->SetObject(nullptr);
}

void SBlueprintEditorValidationTab::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	UpdateBlueprintView(true);
}

void SBlueprintEditorValidationTab::UpdateBlueprintView(bool bForceRefresh)
{
	DetailsView->SetObject(BlueprintView.Get(), bForceRefresh);
}

void SBlueprintEditorValidationTab::OnBlueprintChanged(UBlueprint* Blueprint)
{
	UpdateBlueprintView(true);
}

void FBlueprintEditorValidationTabLayout::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailLayout.GetObjectsBeingCustomized(Objects);
	
	if (Objects.Num() == 0)
	{
		return;
	}
	UObject* EditedObject = Objects[0].Get();

	UBlueprint* Blueprint = BlueprintEditor.Pin()->GetBlueprintObj();
	if (Blueprint == nullptr)
	{
		return;
	}

	static const FName ComponentsCategoryName = TEXT("Components");
	static const FName PropertiesCategoryName = TEXT("Properties");
	IDetailCategoryBuilder& ComponentsCategory = DetailLayout.EditCategory(ComponentsCategoryName,
		LOCTEXT("ComponentCategoryTitle", "Components"), ECategoryPriority::Important).InitiallyCollapsed(false);
	IDetailCategoryBuilder& PropertiesCategory = DetailLayout.EditCategory(PropertiesCategoryName,
		LOCTEXT("PropertyCategoryTitle", "Properties"), ECategoryPriority::Default).InitiallyCollapsed(false);

	UObject* GeneratedObject = Blueprint->GeneratedClass->GetDefaultObject();
	for (const FProperty* Property: TFieldRange<FProperty>{Blueprint->GeneratedClass, EFieldIterationFlags::None})
	{
		if (UE::AssetValidation::CanValidatePropertyValue(Property) || UE::AssetValidation::CanValidatePropertyRecursively(Property))
		{
			TSharedPtr<IPropertyHandle> PropertyHandle = DetailLayout.AddObjectPropertyData({GeneratedObject}, Property->GetFName());
			check(PropertyHandle.IsValid());
			
			const bool bBlueprintComponent = UE::AssetValidation::IsBlueprintComponentProperty(Property);
			
			IDetailCategoryBuilder& Category = bBlueprintComponent ? ComponentsCategory : PropertiesCategory;
			if (bBlueprintComponent)
			{
				auto ComponentBuilder = UE::AssetValidation::FBlueprintComponentCustomization::MakeNodeBuilder(BlueprintEditor.Pin(), PropertyHandle.ToSharedRef());
				Category.AddCustomBuilder(ComponentBuilder.ToSharedRef());
			}
			else
			{
				auto PropertyBuilder = UE::AssetValidation::FBlueprintVariableCustomization::MakeNodeBuilder(BlueprintEditor.Pin(), PropertyHandle.ToSharedRef(), PropertiesCategoryName);
				Category.AddCustomBuilder(PropertyBuilder.ToSharedRef());
			}
		}
	}
}
