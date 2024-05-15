#include "EnginePropertyExtensionCustomization.h"

#include "AssetValidationDefines.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "SPropertySelector.h"
#include "BlueprintVariableCustomization.h"
#include "DetailLayoutBuilder.h"
#include "IDocumentation.h"
#include "PropertyExtensionTypes.h"
#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

namespace UE::AssetValidation
{
	
bool FEnginePropertyExtensionCustomization::FCustomizationTarget::HandleIsMetaVisible(const FName& MetaKey) const
{
	if (Customization.IsValid())
	{
		if (const FProperty* Property = Customization.Pin()->GetProperty())
		{
			return IsPropertyMetaVisible(Property, MetaKey);
		}
	}

	return false;
}

bool FEnginePropertyExtensionCustomization::FCustomizationTarget::HandleIsMetaEditable(FName MetaKey) const
{
	if (Customization.IsValid())
	{
		auto Shared = Customization.Pin();
		if (FProperty* Property = Shared->GetProperty())
		{
			if (MetaKey == UE::AssetValidation::FailureMessage)
			{
				// enable editing FailureMessage only if value validation meta is already set
				const FEnginePropertyExtension& PropertyDesc = Shared->GetPropertyExtension();
				return PropertyDesc.HasMetaData(UE::AssetValidation::Validate) || PropertyDesc.HasMetaData(UE::AssetValidation::ValidateKey) || PropertyDesc.HasMetaData(UE::AssetValidation::ValidateValue);
			}

			return true;
		}
	}

	return false;
}

bool FEnginePropertyExtensionCustomization::FCustomizationTarget::HandleGetMetaState(const FName& MetaKey, FString& OutValue) const
{
	if (Customization.IsValid())
	{
		const FEnginePropertyExtension& PropertyDesc = Customization.Pin()->GetPropertyExtension();
		if (PropertyDesc.HasMetaData(MetaKey))
		{
			OutValue = PropertyDesc.GetMetaData(MetaKey);
			return true;
		}
	}

	return false;
}

void FEnginePropertyExtensionCustomization::FCustomizationTarget::HandleMetaStateChanged(bool NewMetaState, const FName& MetaKey, FString MetaValue)
{
	if (Customization.IsValid())
	{
		auto Shared = Customization.Pin();
		Shared->StructHandle->NotifyPreChange();
	
		FEnginePropertyExtension& PropertyDesc = Shared->GetPropertyExtension();
		if (NewMetaState)
		{
			PropertyDesc.SetMetaData(MetaKey, MetaValue);
		}
		else
		{
			PropertyDesc.RemoveMetaData(MetaKey);
		}
	
		Shared->StructHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	}
}

void FEnginePropertyExtensionCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// cache property utilities to update children layout when property value changes
	PropertyUtilities = CustomizationUtils.GetPropertyUtilities();

	// cache customized object
	TArray<UObject*> Objects;
	StructPropertyHandle->GetOuterObjects(Objects);
	CustomizedObject = Objects[0];

	StructHandle = StructPropertyHandle.ToSharedPtr();
	PropertyPathHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEnginePropertyExtension, PropertyPath));
	PropertyPathHandle->MarkHiddenByCustomization();

	MetaDataMapHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEnginePropertyExtension, MetaDataMap));
	MetaDataMapHandle->MarkHiddenByCustomization();

	StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEnginePropertyExtension, Struct))
	->MarkHiddenByCustomization();

	HeaderRow
	.ShouldAutoExpand(true)
	.NameContent()
	[
		PropertyPathHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		SAssignNew(PropertySelector, SPropertySelector)
		.OnGetStruct(FGetStructEvent::CreateSP(this, &ThisClass::GetOwningStruct))
		.OnGetPropertyPath(FGetPropertyPathEvent::CreateSP(this, &ThisClass::GetPropertyPath))
		.OnPropertySelectionChanged(FPropertySelectionEvent::CreateSP(this, &ThisClass::HandlePropertyChanged))
	];
}

void FEnginePropertyExtensionCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	const FProperty* Property = GetProperty();
	if (Property == nullptr)
	{
		// nothing to customize
		return;
	}

	CustomizationTarget = MakeShared<FCustomizationTarget>(*this);
	CustomizationTarget->CustomizeForObject(CustomizationTarget, [&ChildBuilder](const FText& SearchString) -> FDetailWidgetRow&
	{
		return ChildBuilder.AddCustomRow(SearchString).ShouldAutoExpand(true);
	});

	AddDefaultsEditableRow(ChildBuilder.AddCustomRow(LOCTEXT("IsVariableEditableTitle", "Defaults Editable")));
}

FEnginePropertyExtensionCustomization::~FEnginePropertyExtensionCustomization()
{
	UE_LOG(LogAssetValidation, Verbose, TEXT("FEngineVariableDescCustomization has been destroyed"));
}

void FEnginePropertyExtensionCustomization::AddDefaultsEditableRow(FDetailWidgetRow& WidgetRow)
{
	// @todo: duplicate in FBlueprintVariableCustomization::AddDefaultsEditableRow
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
		.IsEnabled(true)
		.ToolTip(Tooltip)
	];
}

EVisibility FEnginePropertyExtensionCustomization::ShowEditableCheckboxVisibility() const
{
	return EVisibility::Visible;
}

ECheckBoxState FEnginePropertyExtensionCustomization::OnEditableCheckboxState() const
{
	return GetPropertyExtension().HasMetaData(UE::AssetValidation::DisableEditOnTemplate) ? ECheckBoxState::Unchecked : ECheckBoxState::Checked;
}

void FEnginePropertyExtensionCustomization::OnEditableChanged(ECheckBoxState InNewState)
{
	FEnginePropertyExtension& PropertyExtension = GetPropertyExtension();
	if (InNewState == ECheckBoxState::Checked)
	{
		PropertyExtension.RemoveMetaData(UE::AssetValidation::DisableEditOnTemplate);
	}
	else
	{
		PropertyExtension.SetMetaData(UE::AssetValidation::DisableEditOnTemplate, {});
	}
}

void FEnginePropertyExtensionCustomization::HandlePropertyChanged(TFieldPath<FProperty> NewPath)
{
	// notify pre change to a struct value
	StructHandle->NotifyPreChange();

	FEnginePropertyExtension& PropertyExtension = GetPropertyExtension();
	PropertyExtension.PropertyPath = NewPath.Get(PropertyExtension.Struct);
	PropertyExtension.MetaDataMap.Empty();

	// notify post change to a struct value
	StructHandle->NotifyPostChange(EPropertyChangeType::ValueSet);

	if (PropertyUtilities.IsValid())
	{
		// notify UI to refresh itself
		PropertyUtilities->RequestForceRefresh();
	}
}

FProperty* FEnginePropertyExtensionCustomization::GetProperty() const
{
	return GetPropertyExtension().GetProperty();
}

TFieldPath<FProperty> FEnginePropertyExtensionCustomization::GetPropertyPath() const
{
	return GetPropertyExtension().PropertyPath;
}

UStruct* FEnginePropertyExtensionCustomization::GetOwningStruct() const
{
	return GetPropertyExtension().Struct;
}

FEnginePropertyExtension& FEnginePropertyExtensionCustomization::GetPropertyExtension() const
{
	if (uint8* StructValue = StructHandle->GetValueBaseAddress(reinterpret_cast<uint8*>(CustomizedObject.Get())))
	{
		return *reinterpret_cast<FEnginePropertyExtension*>(StructValue);
	}

	static FEnginePropertyExtension InvalidDesc;
	UE_LOG(LogAssetValidation, Error, TEXT("FEngineVariableDescCustomization: Failed to get struct value"));
	return InvalidDesc;
}
	
} // UE::AssetValidation

#undef LOCTEXT_NAMESPACE