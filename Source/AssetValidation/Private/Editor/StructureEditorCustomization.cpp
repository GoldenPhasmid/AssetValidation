#include "StructureEditorCustomization.h"

#include "AssetValidationDefines.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "PropertyValidationDetailsBuilder.h"
#include "Engine/UserDefinedStruct.h"
#include "PropertyValidators/PropertyValidation.h"

void SStructureEditorValidationTab::Construct(const FArguments& Args)
{
	UserDefinedStruct = Args._Struct;

	StructScope = MakeShared<FStructOnScope>(UserDefinedStruct.Get());
	UserDefinedStruct->InitializeDefaultValue(StructScope->GetStructMemory());
	StructScope->SetPackage(UserDefinedStruct->GetPackage());
	FPropertyEditorModule& PropertyEditor = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.bShowOptions = false;

	DetailsView = PropertyEditor.CreateDetailView(DetailsViewArgs);
	auto CustomLayoutDelegate = FOnGetDetailCustomizationInstance::CreateStatic(&FStructureEditorValidationTabLayout::MakeInstance, StructScope);
	DetailsView->RegisterInstancedCustomPropertyLayout(UUserDefinedStruct::StaticClass(), CustomLayoutDelegate);
		
	DetailsView->SetObject(UserDefinedStruct.Get());

	ChildSlot
	[
		DetailsView.ToSharedRef()
	];
}

void SStructureEditorValidationTab::PreChange(const UUserDefinedStruct* Struct, FStructureEditorUtils::EStructureEditorChangeInfo Info)
{
	if (Info != FStructureEditorUtils::DefaultValueChanged)
	{
		StructScope->Destroy();
		DetailsView->SetObject(nullptr);
	}
}

void SStructureEditorValidationTab::PostChange(const UUserDefinedStruct* Struct, FStructureEditorUtils::EStructureEditorChangeInfo Info)
{
	if (Info != FStructureEditorUtils::DefaultValueChanged)
	{
		StructScope->Initialize(UserDefinedStruct.Get());
		// Force the set object call because we may be called multiple times in a row if more than one struct was changed at the same time
		DetailsView->SetObject(UserDefinedStruct.Get(), true);
	}

	UserDefinedStruct.Get()->InitializeDefaultValue(StructScope->GetStructMemory());
}

void FStructureEditorValidationTabLayout::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailLayout.GetObjectsBeingCustomized(Objects);
	
	if (Objects.Num() == 0)
	{
		return;
	}

	UserDefinedStruct = CastChecked<UUserDefinedStruct>(Objects[0].Get());
	
	IDetailCategoryBuilder& Category = DetailLayout.EditCategory("Validation",
	NSLOCTEXT("AssetValidation", "ValidationDetailsCategory", "Validation"), ECategoryPriority::Default).InitiallyCollapsed(false);

	for (const FProperty* Property: TFieldRange<FProperty>{UserDefinedStruct.Get(), EFieldIterationFlags::Default})
	{
		if (UE::AssetValidation::CanValidatePropertyValue(Property) || UE::AssetValidation::CanValidatePropertyRecursively(Property))
		{
			TSharedPtr<IPropertyHandle> PropertyHandle = DetailLayout.AddStructurePropertyData(StructScope, Property->GetFName());
			check(PropertyHandle.IsValid());
			
			TSharedRef<IDetailCustomNodeBuilder> PropertyBuilder = MakeShared<FPropertyValidationDetailsBuilder>(UserDefinedStruct.Get(), PropertyHandle.ToSharedRef());
			Category.AddCustomBuilder(PropertyBuilder);
		}
	}
}

FStructureEditorValidationTabLayout::~FStructureEditorValidationTabLayout()
{
	UE_LOG(LogAssetValidation, Verbose, TEXT("FUserDefinedStructValidationDetails has been destroyed."));
}


