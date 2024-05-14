#include "BlueprintEditorCustomization.h"

#include "BlueprintEditor.h"
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
	DetailsView->SetObject(BlueprintView.Get());

	ChildSlot
	[
		DetailsView.ToSharedRef()
	];
}

void SBlueprintEditorValidationTab::NotifyPreChange(FProperty* PropertyAboutToChange)
{
	DetailsView->SetObject(nullptr);
}

void SBlueprintEditorValidationTab::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
#if 0
	DetailsView->SetObject(Blueprint.Get(), true);
#endif
	DetailsView->SetObject(BlueprintView.Get(), true);
}

void SBlueprintEditorValidationTab::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(BlueprintView);
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

	IDetailCategoryBuilder& ComponentsCategory = DetailLayout.EditCategory("Components",
		LOCTEXT("ComponentCategoryTitle", "Components"), ECategoryPriority::Important).InitiallyCollapsed(false);
	IDetailCategoryBuilder& PropertiesCategory = DetailLayout.EditCategory("Properties",
		LOCTEXT("PropertyCategoryTitle", "Properties"), ECategoryPriority::Default).InitiallyCollapsed(false);

	UObject* GeneratedObject = Blueprint->GeneratedClass->GetDefaultObject();
	for (const FProperty* Property: TFieldRange<FProperty>{Blueprint->GeneratedClass, EFieldIterationFlags::None})
	{
		if (UE::AssetValidation::CanValidatePropertyValue(Property) || UE::AssetValidation::CanValidatePropertyRecursively(Property))
		{
			IDetailCategoryBuilder& Category = UE::AssetValidation::IsBlueprintComponentProperty(Property) ? ComponentsCategory : PropertiesCategory;
			
			TSharedPtr<IPropertyHandle> PropertyHandle = DetailLayout.AddObjectPropertyData({GeneratedObject}, Property->GetFName());
			check(PropertyHandle.IsValid());
			
			TSharedRef<IDetailCustomNodeBuilder> PropertyBuilder = MakeShared<FPropertyValidationDetailsBuilder>(GeneratedObject, PropertyHandle.ToSharedRef(), false);
			Category.AddCustomBuilder(PropertyBuilder);
		}
	}
}
