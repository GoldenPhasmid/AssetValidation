#include "CustomizationTarget.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyValidationSettings.h"
#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

void UE::AssetValidation::ICustomizationTarget::CustomizeForObject(TSharedPtr<UE::AssetValidation::ICustomizationTarget> Target, TFunctionRef<FDetailWidgetRow&(const FText&)> GetCustomRow)
{
	using ICustomizationTarget = UE::AssetValidation::ICustomizationTarget;
	const FSlateFontInfo DetailFont = IDetailLayoutBuilder::GetDetailFont();

	// create Validate checkbox
	GetCustomRow(LOCTEXT("ValidateRowName", "Validate"))
	.Visibility(TAttribute<EVisibility>::CreateSP(Target.Get(), &ICustomizationTarget::GetMetaVisibility, UE::AssetValidation::Validate))
	.IsEnabled(TAttribute<bool>::CreateSP(Target.Get(), &ICustomizationTarget::IsMetaEditable, UE::AssetValidation::Validate))
	.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("ValidateLabel", "Validate Variable"))
		.ToolTipText(LOCTEXT("ValidateToolTip", "Should this variable value validate on save?"))
		.Font(DetailFont)
	]
	.ValueContent()
	[
		SNew(SCheckBox)
		.IsChecked(Target.Get(), &ICustomizationTarget::GetMetaState, UE::AssetValidation::Validate)
		.OnCheckStateChanged(Target.Get(), &ICustomizationTarget::SetMetaState, UE::AssetValidation::Validate)
	];

	// create ValidateKey checkbox for map properties
	GetCustomRow(LOCTEXT("ValidateKeyRowName", "ValidateKey"))
	.Visibility(TAttribute<EVisibility>::CreateSP(Target.Get(), &ICustomizationTarget::GetMetaVisibility, UE::AssetValidation::ValidateKey))
	.IsEnabled(TAttribute<bool>::CreateSP(Target.Get(), &ICustomizationTarget::IsMetaEditable, UE::AssetValidation::ValidateKey))
	.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("ValidateKeyLabel", "Validate Map Keys"))
		.ToolTipText(LOCTEXT("ValidateKeyToolTip", "Should map keys validate on save?"))
		.Font(DetailFont)
	]
	.ValueContent()
	[
		SNew(SCheckBox)
		.IsChecked(Target.Get(), &ICustomizationTarget::GetMetaState, UE::AssetValidation::ValidateKey)
		.OnCheckStateChanged(Target.Get(), &ICustomizationTarget::SetMetaState, UE::AssetValidation::ValidateKey)
	];

	// create ValidateValue checkbox for map properties
	GetCustomRow(LOCTEXT("ValidateValueRowName", "ValidateValue"))
	.Visibility(TAttribute<EVisibility>::CreateSP(Target.Get(), &ICustomizationTarget::GetMetaVisibility, UE::AssetValidation::ValidateValue))
	.IsEnabled(TAttribute<bool>::CreateSP(Target.Get(), &ICustomizationTarget::IsMetaEditable, UE::AssetValidation::ValidateValue))
	.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("ValidateValueLabel", "Validate Map Values"))
		.ToolTipText(LOCTEXT("ValidateValueToolTip", "Should map values be validate on save?"))
		.Font(DetailFont)
	]
	.ValueContent()
	[
		SNew(SCheckBox)
		.IsChecked(Target.Get(), &ICustomizationTarget::GetMetaState, UE::AssetValidation::ValidateValue)
		.OnCheckStateChanged(Target.Get(), &ICustomizationTarget::SetMetaState, UE::AssetValidation::ValidateValue)
	];

	// create ValidateRecursive checkbox for simple object properties and objects inside containers
	GetCustomRow(LOCTEXT("ValidateRecursiveRowName", "ValidateRecursive"))
	.Visibility(TAttribute<EVisibility>::CreateSP(Target.Get(), &ICustomizationTarget::GetMetaVisibility, UE::AssetValidation::ValidateRecursive))
	.IsEnabled(TAttribute<bool>::CreateSP(Target.Get(), &ICustomizationTarget::IsMetaEditable, UE::AssetValidation::ValidateRecursive))
	.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("ValidateRecursiveLabel", "Validate Inner Properties"))
		.ToolTipText(LOCTEXT("ValidateRecursiveToolTip", "Should this variable's inner properties validate on save? Appears for objects and objects inside containers."))
		.Font(DetailFont)
	]
	.ValueContent()
	[
		SNew(SCheckBox)
		.IsChecked(Target.Get(), &ICustomizationTarget::GetMetaState, UE::AssetValidation::ValidateRecursive)
		.OnCheckStateChanged(Target.Get(), &ICustomizationTarget::SetMetaState, UE::AssetValidation::ValidateRecursive)
	];
	
	// create FailureMessage text box. Appears if property value can be validated
	GetCustomRow(LOCTEXT("ValidateFailureMessage", "FailureMessage"))
	.Visibility(TAttribute<EVisibility>::CreateSP(Target.Get(), &ICustomizationTarget::GetMetaVisibility, UE::AssetValidation::FailureMessage))
	.IsEnabled(TAttribute<bool>::CreateSP(Target.Get(), &ICustomizationTarget::IsMetaEditable, UE::AssetValidation::FailureMessage))
	.NameContent()
	[
		SNew(STextBlock)
		.Text(LOCTEXT("FailureMessage", "Validation Failed Message"))
		.ToolTipText(LOCTEXT("FailureMessageToolTip", "Message that is being displayed if validation for this property value has failed, instead of default one.\r\nFor example: \'always set storage type for storage component\' instead of \'object property not set\'"))
		.Font(DetailFont)
	]
	.ValueContent()
	.MinDesiredWidth(250.f)
	.MaxDesiredWidth(250.f)
	[
		SNew(SEditableTextBox)
		.IsEnabled(Target.Get(), &ICustomizationTarget::IsMetaEditable, UE::AssetValidation::FailureMessage)
		.Text(Target.Get(), &ICustomizationTarget::GetMetaValue, UE::AssetValidation::FailureMessage)
		.ToolTipText(Target.Get(), &ICustomizationTarget::GetMetaValue, UE::AssetValidation::FailureMessage)
		.OnTextCommitted(Target.Get(), &ICustomizationTarget::SetMetaValue, UE::AssetValidation::FailureMessage)
		.Font(DetailFont)
	];
}

bool UE::AssetValidation::ICustomizationTarget::IsPropertyMetaVisible(const FProperty* Property, const FName& MetaKey) const
{
	if (Property->IsA<FMapProperty>() && MetaKey == UE::AssetValidation::Validate)
	{
		// should "ValidateKey" and "ValidateValue" for map properties, hide "Validate"
		return false;
	}
	if (Property->IsA<FStructProperty>() && MetaKey == UE::AssetValidation::ValidateRecursive && UPropertyValidationSettings::Get()->bAutoValidateStructInnerProperties)
	{
		// don't should "ValidateRecursive" for struct properties if auto validation is enabled
		return false;
	}
	
	return UE::AssetValidation::CanApplyMeta(Property, MetaKey);
}

#undef LOCTEXT_NAMESPACE
