#include "BlueprintComponentCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "SSubobjectEditor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "PropertyValidators/PropertyValidation.h"

namespace UE::AssetValidation
{

TSharedPtr<IDetailCustomization> FBlueprintComponentCustomization::MakeInstance(
	TSharedPtr<FBlueprintEditor> InBlueprintEditor,
	FOnGetDetailCustomizationInstance ChildDelegate)
{
	return MakeShared<ThisClass>(InBlueprintEditor, ChildDelegate, FPrivateToken{});
}

TSharedPtr<IDetailCustomNodeBuilder> FBlueprintComponentCustomization::MakeNodeBuilder(
	TSharedPtr<FBlueprintEditor> InBlueprintEditor,
	TSharedPtr<IPropertyHandle> PropertyHandle)
{
	return MakeShared<ThisClass>(InBlueprintEditor, PropertyHandle, FPrivateToken{});
}

FBlueprintComponentCustomization::FBlueprintComponentCustomization(TSharedPtr<FBlueprintEditor> InBlueprintEditor, FOnGetDetailCustomizationInstance InChildDelegate, FPrivateToken)
	: BlueprintEditor(InBlueprintEditor)
	, Blueprint(InBlueprintEditor->GetBlueprintObj())
	, ChildCustomizationDelegate(InChildDelegate)
{
	
}

FBlueprintComponentCustomization::FBlueprintComponentCustomization(TSharedPtr<FBlueprintEditor> InBlueprintEditor, TSharedPtr<IPropertyHandle> InPropertyHandle, FPrivateToken)
	: BlueprintEditor(InBlueprintEditor)
	, Blueprint(InBlueprintEditor->GetBlueprintObj())
	, PropertyHandle(InPropertyHandle)
{
	
}
	
FBlueprintComponentCustomization::~FBlueprintComponentCustomization()
{
	CustomizationTarget.Reset();
}
	
void FBlueprintComponentCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	if (ChildCustomizationDelegate.IsBound())
	{
		// handle child customization
		ChildCustomization = ChildCustomizationDelegate.Execute();
		ChildCustomization->CustomizeDetails(DetailLayout);
	}
	
	check(BlueprintEditor.IsValid());
	TSharedPtr<SSubobjectEditor> Editor = BlueprintEditor.Pin()->GetSubobjectEditor();
	check(Editor.IsValid());

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
		ECategoryPriority::Variable).InitiallyCollapsed(false);

	CustomizationTarget = MakeShared<FCustomizationTarget>(*this);
	CustomizationTarget->CustomizeForObject(CustomizationTarget, [&Category](const FText& SearchString) -> FDetailWidgetRow&
	{
		return Category.AddCustomRow(SearchString).ShouldAutoExpand(true);
	});	
}
	
void FBlueprintComponentCustomization::GenerateHeaderRowContent(FDetailWidgetRow& NodeRow)
{
	check(PropertyHandle.IsValid());
	if (const FProperty* Property = PropertyHandle->GetProperty())
	{
		if (UBlueprintGeneratedClass* BlueprintClass = Cast<UBlueprintGeneratedClass>(Property->GetOwnerClass()))
		{
			OwnerBlueprint = Cast<UBlueprint>(BlueprintClass->ClassGeneratedBy);
		}
	}
	
	NodeRow
	.ShouldAutoExpand(true)
	.WholeRowContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	];
}

void FBlueprintComponentCustomization::GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder)
{
	CustomizationTarget = MakeShared<FCustomizationTarget>(*this);
	CustomizationTarget->CustomizeForObject(CustomizationTarget, [&ChildrenBuilder](const FText& SearchString) -> FDetailWidgetRow&
	{
		return ChildrenBuilder.AddCustomRow(SearchString).ShouldAutoExpand(true);
	});	
}

bool FBlueprintComponentCustomization::FCustomizationTarget::HandleIsMetaVisible(const FName& MetaKey) const
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

bool FBlueprintComponentCustomization::FCustomizationTarget::HandleIsMetaEditable(FName MetaKey) const
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

bool FBlueprintComponentCustomization::FCustomizationTarget::HandleGetMetaState(const FName& MetaKey, FString& OutValue) const
{
	return Customization.IsValid() && Customization.Pin()->GetMetaData(MetaKey, OutValue);
}

void FBlueprintComponentCustomization::FCustomizationTarget::HandleMetaStateChanged(bool NewMetaState, const FName& MetaKey, FString MetaValue)
{
	if (Customization.IsValid())
	{
		Customization.Pin()->SetMetaData(MetaKey, NewMetaState, MetaValue);
	}
}

bool FBlueprintComponentCustomization::IsInheritedComponent() const
{
	if (EditingNode.IsValid())
	{
		const FSubobjectData* DataSource = EditingNode->GetDataSource();
		return DataSource->IsInheritedComponent() || DataSource->IsNativeComponent();
	}
	if (PropertyHandle.IsValid())
	{
		return Blueprint.Get() != OwnerBlueprint.Get();
	}

	checkNoEntry();
	return false;
	
}

bool FBlueprintComponentCustomization::HasMetaData(const FName& MetaName) const
{
	FString MetaValue{};
	return GetMetaData(MetaName, MetaValue);
}

bool FBlueprintComponentCustomization::GetMetaData(const FName& MetaName, FString& OutValue) const
{
	if (const FName VariableName = GetVariableName(); VariableName != NAME_None)
	{
		if (FBlueprintEditorUtils::GetBlueprintVariableMetaData(Blueprint.Get(), VariableName, nullptr, MetaName, OutValue))
		{
			return true;
		}
	}

	return false;
}

void FBlueprintComponentCustomization::SetMetaData(const FName& MetaName, bool bEnabled, const FString& MetaValue)
{
	if (const FName VariableName = GetVariableName(); VariableName != NAME_None)
	{
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

FName FBlueprintComponentCustomization::GetVariableName() const
{
	if (Blueprint.IsValid())
	{
		if (EditingNode.IsValid())
		{
			return EditingNode->GetVariableName();
		}
		if (PropertyHandle.IsValid())
		{
			return PropertyHandle->GetProperty()->GetFName();
		}
	}

	return NAME_None;
}
	
} // UE::AssetValidation
