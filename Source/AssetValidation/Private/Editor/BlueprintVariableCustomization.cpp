#include "BlueprintVariableCustomization.h"

#include "BlueprintEditorModule.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "EdMode.h"
#include "IDetailChildrenBuilder.h"
#include "IDocumentation.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

namespace UE::AssetValidation
{

bool FBlueprintVariableCustomization::FCustomizationTarget::HandleIsMetaVisible(const FName& MetaKey) const
{
	if (Customization.IsValid())
	{
		if (const FProperty* Property = Customization.Pin()->CachedProperty.Get())
		{
			return IsPropertyMetaVisible(Property, MetaKey);
		}
	}

	return false;
}

bool FBlueprintVariableCustomization::FCustomizationTarget::HandleIsMetaEditable(FName MetaKey) const
{
	if (Customization.IsValid() && Customization.Pin()->IsVariableInBlueprint())
	{
		auto Shared = Customization.Pin();
		if (MetaKey == UE::AssetValidation::FailureMessage)
		{
			return	Shared->HasMetaData(UE::AssetValidation::Validate) ||
					Shared->HasMetaData(UE::AssetValidation::ValidateKey) ||
					Shared->HasMetaData(UE::AssetValidation::ValidateValue);
		}

		return true;
	}

	return false;
}

bool FBlueprintVariableCustomization::FCustomizationTarget::HandleGetMetaState(const FName& MetaKey, FString& OutValue) const
{
	return Customization.IsValid() && Customization.Pin()->GetMetaData(MetaKey, OutValue);
}

void FBlueprintVariableCustomization::FCustomizationTarget::HandleMetaStateChanged(bool NewMetaState, const FName& MetaKey, FString MetaValue)
{
	if (Customization.IsValid())
	{
		Customization.Pin()->SetMetaData(MetaKey, NewMetaState, MetaValue);
	}
}

TSharedPtr<IDetailCustomization> FBlueprintVariableCustomization::MakeInstance(TSharedPtr<IBlueprintEditor> InBlueprintEditor)
{
	if (InBlueprintEditor.IsValid())
	{
		const TArray<UObject*>* Objects = InBlueprintEditor->GetObjectsCurrentlyBeingEdited();
		if (Objects && Objects->Num() == 1)
		{
			if (UBlueprint* Blueprint = Cast<UBlueprint>((*Objects)[0]))
			{
				return MakeShared<ThisClass>(InBlueprintEditor, Blueprint, FPrivateToken{});
			}
		}
	}

	return nullptr;
}

TSharedPtr<IDetailCustomNodeBuilder> FBlueprintVariableCustomization::MakeNodeBuilder(TSharedPtr<IBlueprintEditor> InBlueprintEditor, TSharedRef<IPropertyHandle> InPropertyHandle, FName CategoryName)
{
	const TArray<UObject*>* Objects = InBlueprintEditor->GetObjectsCurrentlyBeingEdited();
	if (Objects && Objects->Num() == 1)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>((*Objects)[0]))
		{
			return MakeShared<ThisClass>(InBlueprintEditor, Blueprint, InPropertyHandle, CategoryName, FPrivateToken{});
		}
	}

	return nullptr;
}

void FBlueprintVariableCustomization::AddDefaultsEditableRow(FDetailWidgetRow& WidgetRow)
{
	// @todo: duplicate in FEnginePropertyExtensionCustomization::AddDefaultsEditableRow
	const FText TooltipText = LOCTEXT("VarEditableTooltip", "Adds CPF_DisableEditOnTemplate flag to property, allows validation subsystem to skip it on default objects.");
	TSharedPtr<SToolTip> Tooltip = IDocumentation::Get()->CreateToolTip(TooltipText, nullptr, {}, {});

	WidgetRow
	.Visibility(TAttribute<EVisibility>(this, &ThisClass::ShowEditableCheckboxVisibility))
	.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("IsVariableEditableTitle", "Defaults Editable"))
		.ToolTip(Tooltip)
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	[
		SNew(SCheckBox)
		.IsChecked(this, &ThisClass::OnEditableCheckboxState)
		.OnCheckStateChanged(this, &ThisClass::OnEditableChanged)
		.IsEnabled(IsVariableInBlueprint())
		.ToolTip(Tooltip)
	];
}

EVisibility FBlueprintVariableCustomization::ShowEditableCheckboxVisibility() const
{
	if (IsVariableInBlueprint())
	{
		return EVisibility::Visible;
	}
	return EVisibility::Collapsed;
}

ECheckBoxState FBlueprintVariableCustomization::OnEditableCheckboxState() const
{
	if (CachedProperty.IsValid())
	{
		return CachedProperty->HasAnyPropertyFlags(CPF_DisableEditOnTemplate) ? ECheckBoxState::Unchecked : ECheckBoxState::Checked;
	}
	return ECheckBoxState::Unchecked;
}

void FBlueprintVariableCustomization::OnEditableChanged(ECheckBoxState InNewState)
{
	if (!CachedProperty.IsValid() || !Blueprint.IsValid())
	{
		return;
	}
	
	FName VarName = CachedProperty->GetFName();

	// Toggle the flag on the blueprint's version of the variable description, based on state
	const bool bVariableIsExposed = InNewState == ECheckBoxState::Checked;
	
	const int32 VarIndex = FBlueprintEditorUtils::FindNewVariableIndex(Blueprint.Get(), VarName);

	if (!bVariableIsExposed)
	{
		FBlueprintEditorUtils::RemoveBlueprintVariableMetaData(Blueprint.Get(), VarName, nullptr, FEdMode::MD_MakeEditWidget);
	}

	if (VarIndex != INDEX_NONE)
	{
		if(!bVariableIsExposed)
		{
			Blueprint->NewVariables[VarIndex].PropertyFlags |= CPF_DisableEditOnTemplate;
		}
		else
		{
			Blueprint->NewVariables[VarIndex].PropertyFlags &= ~CPF_DisableEditOnTemplate;
		}
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint.Get());
}

void FBlueprintVariableCustomization::Initialize(UObject* EditedObject)
{
	if (PropertyHandle.IsValid())
	{
		CachedProperty = PropertyHandle->GetProperty();
	}
	else if (UPropertyWrapper* PropertyWrapper = Cast<UPropertyWrapper>(EditedObject))
	{
		CachedProperty = PropertyWrapper->GetProperty();
	}

	if (CachedProperty.IsValid())
	{
		if (UBlueprintGeneratedClass* BlueprintClass = Cast<UBlueprintGeneratedClass>(CachedProperty->GetOwnerClass()))
		{
			OwnerBlueprint = Cast<UBlueprint>(BlueprintClass->ClassGeneratedBy);
		}	
	}
}

void FBlueprintVariableCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailLayout.GetObjectsBeingCustomized(Objects);

	if (Objects.Num() == 0)
	{
		return;
	}

	Initialize(Objects[0].Get());

	if (CategoryName == NAME_None)
	{
		CategoryName = TEXT("Validation");
		// this creates default category if it is not yet present
		DetailLayout.EditCategory(
		CategoryName,
		LOCTEXT("ValidationCategoryTitle", "Validation"),
		ECategoryPriority::Variable).InitiallyCollapsed(false);
	}

	IDetailCategoryBuilder& ValidationCategory = DetailLayout.EditCategory(CategoryName);
	
	CustomizationTarget = MakeShared<FCustomizationTarget>(*this);
	CustomizationTarget->CustomizeForObject(CustomizationTarget, FText::FromString(TEXT("Validation")),
		[&ValidationCategory](const FText& SearchString) -> FDetailWidgetRow&
	{
		return ValidationCategory.AddCustomRow(SearchString).ShouldAutoExpand(true);
	});

	IDetailCategoryBuilder& VariableCategory = DetailLayout.EditCategory("Variable");
	AddDefaultsEditableRow(ValidationCategory.AddCustomRow(LOCTEXT("IsVariableEditableTitle", "Defaults Editable")));
}

FBlueprintVariableCustomization::~FBlueprintVariableCustomization()
{
	CustomizationTarget.Reset();
}

void FBlueprintVariableCustomization::GenerateHeaderRowContent(FDetailWidgetRow& NodeRow)
{
	TArray<UObject*> EditedObjects;
	PropertyHandle->GetOuterObjects(EditedObjects);
	Initialize(EditedObjects[0]);
	
	NodeRow
	.ShouldAutoExpand(true)
	.WholeRowContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	];
}

void FBlueprintVariableCustomization::GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder)
{
	CustomizationTarget = MakeShared<FCustomizationTarget>(*this);
	CustomizationTarget->CustomizeForObject(CustomizationTarget, PropertyHandle->GetPropertyDisplayName(),
		[&ChildrenBuilder](const FText& SearchString) -> FDetailWidgetRow&
	{
		return ChildrenBuilder.AddCustomRow(SearchString).ShouldAutoExpand(true);
	});
	
	AddDefaultsEditableRow(ChildrenBuilder.AddCustomRow(LOCTEXT("IsVariableEditableTitle", "Defaults Editable")));
}

bool FBlueprintVariableCustomization::IsVariableInBlueprint() const
{
	return Blueprint.Get() == OwnerBlueprint.Get();
}

bool FBlueprintVariableCustomization::IsVariableInheritedByBlueprint() const
{
	const UClass* PropertyOwnerClass = nullptr;
	if (OwnerBlueprint.IsValid())
	{
		PropertyOwnerClass = OwnerBlueprint->SkeletonGeneratedClass;
	}
	else if (CachedProperty.IsValid())
	{
		PropertyOwnerClass = CachedProperty->GetOwnerClass();
	}
	const UClass* SkeletonGeneratedClass = Blueprint->SkeletonGeneratedClass;
	return SkeletonGeneratedClass && SkeletonGeneratedClass->IsChildOf(PropertyOwnerClass);
}

bool FBlueprintVariableCustomization::HasMetaData(const FName& MetaName) const
{
	FString MetaValue{};
	return GetMetaData(MetaName, MetaValue);
}


bool FBlueprintVariableCustomization::GetMetaData(const FName& MetaName, FString& OutValue) const
{
	if (Blueprint.IsValid() && CachedProperty.IsValid())
	{
		if (FBlueprintEditorUtils::GetBlueprintVariableMetaData(Blueprint.Get(), CachedProperty->GetFName(), nullptr, MetaName, OutValue))
		{
			return true;
		}
	}

	return false;
}

void FBlueprintVariableCustomization::SetMetaData(const FName& MetaName, bool bEnabled, const FString& MetaValue)
{
	if (Blueprint.IsValid() && CachedProperty.IsValid())
	{
		FName VariableName = CachedProperty->GetFName();
		if (bEnabled)
		{
			FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint.Get(), VariableName, nullptr, MetaName, MetaValue);
		}
		else
		{
			FBlueprintEditorUtils::RemoveBlueprintVariableMetaData(Blueprint.Get(), VariableName, nullptr, MetaName);
		}

		if (OnRebuildChildren.IsBound())
		{
			OnRebuildChildren.Execute();
		}
	}
}
	
} // UE::AssetValidation

#undef LOCTEXT_NAMESPACE