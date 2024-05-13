#include "BlueprintEditorCustomization.h"

#include "BlueprintEditor.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "StructureEditorCustomization.h"
#include "PropertyValidators/PropertyValidation.h"

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

#if 1
	IDetailCategoryBuilder& Category = DetailLayout.EditCategory("Validation",
NSLOCTEXT("AssetValidation", "ValidationDetailsCategory", "Validation"), ECategoryPriority::Default).InitiallyCollapsed(false);
#else
	IDetailCategoryBuilder& Category = DetailLayout.EditCategoryAllowNone(NAME_None);
#endif

	UObject* GeneratedObject = Blueprint->GeneratedClass->GetDefaultObject();
	for (const FProperty* Property: TFieldRange<FProperty>{Blueprint->GeneratedClass, EFieldIterationFlags::None})
	{
		if (UE::AssetValidation::CanValidatePropertyValue(Property) || UE::AssetValidation::CanValidatePropertyRecursively(Property))
		{
			TSharedPtr<IPropertyHandle> PropertyHandle = DetailLayout.AddObjectPropertyData({GeneratedObject}, Property->GetFName());
			check(PropertyHandle.IsValid());
			
			TSharedRef<IDetailCustomNodeBuilder> PropertyBuilder = MakeShared<FPropertyValidationDetailsBuilder>(EditedObject, PropertyHandle.ToSharedRef());
			Category.AddCustomBuilder(PropertyBuilder);
		}
	}
}
