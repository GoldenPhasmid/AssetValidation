﻿#include "PropertyValidationBlueprintComponentCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "SSubobjectEditor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "PropertyValidators/PropertyValidation.h"

TSharedPtr<IDetailCustomization> FPropertyValidationBlueprintComponentCustomization::MakeInstance(TSharedPtr<FBlueprintEditor> InBlueprintEditor, FOnGetDetailCustomizationInstance ChildDelegate)
{
	return MakeShared<ThisClass>(InBlueprintEditor, ChildDelegate);
}

FPropertyValidationBlueprintComponentCustomization::FPropertyValidationBlueprintComponentCustomization(TSharedPtr<FBlueprintEditor> InBlueprintEditor, FOnGetDetailCustomizationInstance InChildDelegate)
	: BlueprintEditor(InBlueprintEditor)
	, Blueprint(InBlueprintEditor->GetBlueprintObj())
	, ChildCustomizationDelegate(InChildDelegate)
{
	
}

void FPropertyValidationBlueprintComponentCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	check(BlueprintEditor.IsValid());
	TSharedPtr<SSubobjectEditor> Editor = BlueprintEditor.Pin()->GetSubobjectEditor();
	check(Editor.IsValid());

	if (ChildCustomizationDelegate.IsBound())
	{
		// handle child customization
		ChildCustomization = ChildCustomizationDelegate.Execute();
		ChildCustomization->CustomizeDetails(DetailLayout);
	}

	TArray<FSubobjectEditorTreeNodePtrType> Nodes = Editor->GetSelectedNodes();
	if (Nodes.Num() == 0)
	{
		return;
	}

	EditingNode = Nodes[0];
	check(EditingNode.IsValid());

	IDetailCategoryBuilder& Category = DetailLayout.EditCategory(
		"Validation",
		NSLOCTEXT("AssetValidation", "ValidationCategoryTitle", "Validation"),
		ECategoryPriority::Default).InitiallyCollapsed(false);

	CustomizationTarget = MakeShared<FCustomizationTarget>(*this);
	CustomizationTarget->CustomizeForObject(CustomizationTarget, [&Category](const FText& SearchString) -> FDetailWidgetRow&
	{
		return Category.AddCustomRow(SearchString).ShouldAutoExpand(true);
	});	
}

bool FPropertyValidationBlueprintComponentCustomization::FCustomizationTarget::HandleIsMetaVisible(const FName& MetaKey) const
{
	if (Customization.IsValid())
	{
		// @todo: how do we get component property? Is this even possible? We cant to reuse IsPropertyMetaVisible
		if (MetaKey == UE::AssetValidation::ValidateKey || MetaKey == UE::AssetValidation::ValidateValue)
		{
			return false;
		}

		return true;
	}
	
	return false;
}

bool FPropertyValidationBlueprintComponentCustomization::FCustomizationTarget::HandleIsMetaEditable(FName MetaKey) const
{
	if (Customization.IsValid())
	{
		if (auto Shared = Customization.Pin(); !Shared->IsInheritedComponent())
		{
			if (MetaKey == UE::AssetValidation::FailureMessage)
            {
            	return	Shared->HasMetaData(UE::AssetValidation::Validate)	||
            			Shared->HasMetaData(UE::AssetValidation::ValidateKey) ||
            			Shared->HasMetaData(UE::AssetValidation::ValidateValue);
            }
    
            return true;
		}
	}
	
	return false;
}

bool FPropertyValidationBlueprintComponentCustomization::FCustomizationTarget::HandleGetMetaState(const FName& MetaKey, FString& OutValue) const
{
	return Customization.IsValid() && Customization.Pin()->GetMetaData(MetaKey, OutValue);
}

void FPropertyValidationBlueprintComponentCustomization::FCustomizationTarget::HandleMetaStateChanged(bool NewMetaState, const FName& MetaKey, FString MetaValue)
{
	if (Customization.IsValid())
	{
		Customization.Pin()->SetMetaData(MetaKey, NewMetaState, MetaValue);
	}
}

bool FPropertyValidationBlueprintComponentCustomization::IsInheritedComponent() const
{
	return EditingNode->GetDataSource()->IsInheritedComponent();
}

bool FPropertyValidationBlueprintComponentCustomization::HasMetaData(const FName& MetaName) const
{
	FString MetaValue{};
	return GetMetaData(MetaName, MetaValue);
}

bool FPropertyValidationBlueprintComponentCustomization::GetMetaData(const FName& MetaName, FString& OutValue) const
{
	if (Blueprint.IsValid() && EditingNode.IsValid())
	{
		if (FBlueprintEditorUtils::GetBlueprintVariableMetaData(Blueprint.Get(), EditingNode->GetVariableName(), nullptr, MetaName, OutValue))
		{
			return true;
		}
	}

	return false;
}

void FPropertyValidationBlueprintComponentCustomization::SetMetaData(const FName& MetaName, bool bEnabled, const FString& MetaValue)
{
	if (Blueprint.IsValid() && EditingNode.IsValid())
	{
		const FName VariableName = EditingNode->GetVariableName();
		if (bEnabled)
		{
			FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint.Get(), VariableName, nullptr, MetaName, MetaValue);
		}
		else
		{
			FBlueprintEditorUtils::RemoveBlueprintVariableMetaData(Blueprint.Get(), VariableName, nullptr, MetaName);
		}
	}
}