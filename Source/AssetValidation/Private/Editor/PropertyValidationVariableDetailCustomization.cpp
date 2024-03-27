#include "PropertyValidationVariableDetailCustomization.h"

#include "BlueprintEditorModule.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyValidatorSubsystem.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

bool FPropertyValidationVariableDetailCustomization::FCustomizationTarget::HandleIsMetaVisible(const FName& MetaKey) const
{
	if (Customization.IsValid())
	{
		if (const FProperty* Property = Customization.Pin()->CachedProperty.Get())
		{
			return UE::AssetValidation::CanApplyMeta(Property, MetaKey);
		}
	}
	
	return false;
}

bool FPropertyValidationVariableDetailCustomization::FCustomizationTarget::HandleIsMetaEditable(FName MetaKey) const
{
    if (Customization.IsValid())
    {
        auto Shared = Customization.Pin();
        if (FProperty* Property = Shared->CachedProperty.Get())
        {
        	if (MetaKey == UE::AssetValidation::FailureMessage)
        	{
        		return Property->HasMetaData(UE::AssetValidation::Validate) || Property->HasMetaData(UE::AssetValidation::ValidateKey) || Property->HasMetaData(UE::AssetValidation::ValidateValue);
        	}

        	return true;
        }
    }
	return Customization.IsValid() && Customization.Pin()->IsVariableInBlueprint();
}

bool FPropertyValidationVariableDetailCustomization::FCustomizationTarget::HandleGetMetaState(const FName& MetaKey, FString& OutValue) const
{
	return Customization.IsValid() && Customization.Pin()->GetMetaData(MetaKey, OutValue);
}

void FPropertyValidationVariableDetailCustomization::FCustomizationTarget::HandleMetaStateChanged(bool NewMetaState, const FName& MetaKey, FString MetaValue)
{
	if (Customization.IsValid())
	{
		Customization.Pin()->SetMetaData(MetaKey, NewMetaState, MetaValue);
	}
	
}

TSharedPtr<IDetailCustomization> FPropertyValidationVariableDetailCustomization::MakeInstance(TSharedPtr<IBlueprintEditor> InBlueprintEditor)
{
	if (const TArray<UObject*>* Objects = InBlueprintEditor.IsValid() ? InBlueprintEditor->GetObjectsCurrentlyBeingEdited() : nullptr;
		Objects && Objects->Num() == 1)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>((*Objects)[0]))
		{
			return MakeShared<ThisClass>(InBlueprintEditor, Blueprint);
		}
	}
	
	return nullptr;
}

void FPropertyValidationVariableDetailCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailLayout.GetObjectsBeingCustomized(Objects);

	if (Objects.Num() == 0)
	{
		return;
	}

	UPropertyWrapper* PropertyWrapper = Cast<UPropertyWrapper>(Objects[0].Get());
	if (PropertyWrapper == nullptr)
	{
		return;
	}

	CachedProperty = PropertyWrapper->GetProperty();
	if (!CachedProperty.IsValid())
	{
		return;
	}

	if (UBlueprintGeneratedClass* BlueprintClass = Cast<UBlueprintGeneratedClass>(CachedProperty->GetOwnerClass()))
	{
		PropertyBlueprint = Cast<UBlueprint>(BlueprintClass->ClassGeneratedBy);
	}

	CachedVariableName = CachedProperty->GetFName();
	CachedValidationSubsystem = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	
	IDetailCategoryBuilder& Category = DetailLayout.EditCategory("Validation", LOCTEXT("ValidationDetailsCategory", "Validation"), ECategoryPriority::Default).InitiallyCollapsed(false);
#if 1
	CustomizationTarget = MakeShared<FCustomizationTarget>(*this);
	CustomizationTarget->CustomizeForObject(CustomizationTarget, [&Category](const FText& SearchString) -> FDetailWidgetRow&
	{
		return Category.AddCustomRow(SearchString).ShouldAutoExpand(true);
	});	
#else
	const FSlateFontInfo DetailFont = IDetailLayoutBuilder::GetDetailFont();
	
	// create Validate checkbox
	Category.AddCustomRow(LOCTEXT("ValidateRowName", "Validate"))
	.Visibility(TAttribute<EVisibility>(this, &ThisClass::GetValidateVisibility))
	.IsEnabled(TAttribute<bool>(this, &ThisClass::IsMetaEditingEnabled))
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
		.IsChecked(this, &ThisClass::GetValidateState)
		.OnCheckStateChanged(this, &ThisClass::SetValidateState)
	];

	// create ValidateKey checkbox for map properties
	Category.AddCustomRow(LOCTEXT("ValidateKeyRowName", "ValidateKey"))
	.Visibility(TAttribute<EVisibility>(this, &ThisClass::GetValidateKeyVisibility))
	.IsEnabled(TAttribute<bool>(this, &ThisClass::IsMetaEditingEnabled))
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
		.IsChecked(this, &ThisClass::GetValidateKeyState)
		.OnCheckStateChanged(this, &ThisClass::SetValidateKeyState)
	];

	// create ValidateValue checkbox for map properties
	Category.AddCustomRow(LOCTEXT("ValidateValueRowName", "ValidateValue"))
	.Visibility(TAttribute<EVisibility>(this, &ThisClass::GetValidateValueVisibility))
	.IsEnabled(TAttribute<bool>(this, &ThisClass::IsMetaEditingEnabled))
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
		.IsChecked(this, &ThisClass::GetValidateValueState)
		.OnCheckStateChanged(this, &ThisClass::SetValidateValueState)
	];

	// create ValidateRecursive checkbox for simple object properties and objects inside containers
	Category.AddCustomRow(LOCTEXT("ValidateRecursiveRowName", "ValidateRecursive"))
	.Visibility(TAttribute<EVisibility>(this, &ThisClass::GetValidateRecursiveVisibility))
	.IsEnabled(TAttribute<bool>(this, &ThisClass::IsMetaEditingEnabled))
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
		.IsChecked(this, &ThisClass::GetValidateRecursiveState)
		.OnCheckStateChanged(this, &ThisClass::SetValidateRecursiveState)
	];

	// create FailureMessage text box. Appears if property value can be validated
	Category.AddCustomRow(LOCTEXT("ValidateFailureMessage", "FailureMessage"))
	.Visibility(TAttribute<EVisibility>(this, &ThisClass::IsFailureMessageVisible))
	.IsEnabled(TAttribute<bool>(this, &ThisClass::IsMetaEditingEnabled))
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
		.IsEnabled(this, &ThisClass::IsFailureMessageEnabled)
		.Text(this, &ThisClass::GetFailureMessage)
		.ToolTipText(this, &ThisClass::GetFailureMessage)
		.OnTextCommitted(this, &ThisClass::SetFailureMessage)
		.Font(DetailFont)
	];
#endif
}

bool FPropertyValidationVariableDetailCustomization::IsVariableInBlueprint() const
{
	return Blueprint.Get() == PropertyBlueprint.Get();
}

bool FPropertyValidationVariableDetailCustomization::IsVariableInheritedByBlueprint() const
{
	const UClass* PropertyOwnerClass = nullptr;
	if (PropertyBlueprint.IsValid())
	{
		PropertyOwnerClass = PropertyBlueprint->SkeletonGeneratedClass;
	}
	else if (CachedProperty.IsValid())
	{
		PropertyOwnerClass = CachedProperty->GetOwnerClass();
	}
	const UClass* SkeletonGeneratedClass = Blueprint->SkeletonGeneratedClass;
	return SkeletonGeneratedClass && SkeletonGeneratedClass->IsChildOf(PropertyOwnerClass);
}

bool FPropertyValidationVariableDetailCustomization::HasMetaData(const FName& MetaName) const
{
	FString MetaValue{};
	return GetMetaData(MetaName, MetaValue);
}


bool FPropertyValidationVariableDetailCustomization::GetMetaData(const FName& MetaName, FString& OutValue) const
{
	if (Blueprint.IsValid() && CachedProperty.IsValid())
	{
		if (FBlueprintEditorUtils::GetBlueprintVariableMetaData(Blueprint.Get(), CachedVariableName, nullptr, MetaName, OutValue))
		{
			return true;
		}
	}

	return false;
}

void FPropertyValidationVariableDetailCustomization::SetMetaData(const FName& MetaName, bool bEnabled, const FString& MetaValue)
{
	if (Blueprint.IsValid() && CachedProperty.IsValid())
	{
		if (bEnabled)
		{
			FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint.Get(), CachedVariableName, nullptr, MetaName, MetaValue);
		}
		else
		{
			FBlueprintEditorUtils::RemoveBlueprintVariableMetaData(Blueprint.Get(), CachedVariableName, nullptr, MetaName);
		}
	}
}

bool FPropertyValidationVariableDetailCustomization::IsMetaEditingEnabled() const
{
	return IsVariableInBlueprint();
}

EVisibility FPropertyValidationVariableDetailCustomization::GetValidateVisibility() const
{
	if (const FProperty* Property = CachedProperty.Get())
	{
		return UE::AssetValidation::CanApplyMeta_Validate(Property) ? EVisibility::Visible : EVisibility::Collapsed;
	}
	
	return EVisibility::Collapsed;
}

ECheckBoxState FPropertyValidationVariableDetailCustomization::GetValidateState() const
{
	return HasMetaData(UE::AssetValidation::Validate) ? ECheckBoxState::Checked: ECheckBoxState::Unchecked;
}

void FPropertyValidationVariableDetailCustomization::SetValidateState(ECheckBoxState NewState)
{
	SetMetaData(UE::AssetValidation::Validate, NewState == ECheckBoxState::Checked);
}

ECheckBoxState FPropertyValidationVariableDetailCustomization::GetValidateRecursiveState() const
{
	return HasMetaData(UE::AssetValidation::ValidateRecursive) ? ECheckBoxState::Checked: ECheckBoxState::Unchecked;
}

void FPropertyValidationVariableDetailCustomization::SetValidateRecursiveState(ECheckBoxState NewState)
{
	SetMetaData(UE::AssetValidation::ValidateRecursive, NewState == ECheckBoxState::Checked);
}

ECheckBoxState FPropertyValidationVariableDetailCustomization::GetValidateKeyState() const
{
	return HasMetaData(UE::AssetValidation::ValidateKey) ? ECheckBoxState::Checked: ECheckBoxState::Unchecked;
}

void FPropertyValidationVariableDetailCustomization::SetValidateKeyState(ECheckBoxState NewState)
{
	SetMetaData(UE::AssetValidation::Validate, false);
	SetMetaData(UE::AssetValidation::ValidateKey, NewState == ECheckBoxState::Checked);
}

ECheckBoxState FPropertyValidationVariableDetailCustomization::GetValidateValueState() const
{
	return HasMetaData(UE::AssetValidation::ValidateValue) ? ECheckBoxState::Checked: ECheckBoxState::Unchecked;
}

void FPropertyValidationVariableDetailCustomization::SetValidateValueState(ECheckBoxState NewState)
{
	SetMetaData(UE::AssetValidation::Validate, false);
	SetMetaData(UE::AssetValidation::ValidateValue, NewState == ECheckBoxState::Checked);
}

EVisibility FPropertyValidationVariableDetailCustomization::IsFailureMessageVisible() const
{
	bool bVisible = GetValidateVisibility() == EVisibility::Visible ||
					GetValidateKeyVisibility() == EVisibility::Visible ||
					GetValidateValueVisibility() == EVisibility::Visible;
	return bVisible ? EVisibility::Visible : EVisibility::Collapsed;
}

bool FPropertyValidationVariableDetailCustomization::IsFailureMessageEnabled() const
{
	return GetValidateState() == ECheckBoxState::Checked || GetValidateKeyState() == ECheckBoxState::Checked || GetValidateValueState() == ECheckBoxState::Checked;
}

FText FPropertyValidationVariableDetailCustomization::GetFailureMessage() const
{
	FString Value{};
	FBlueprintEditorUtils::GetBlueprintVariableMetaData(Blueprint.Get(), CachedVariableName, nullptr, UE::AssetValidation::FailureMessage, Value);

	return FText::FromString(Value);
}

void FPropertyValidationVariableDetailCustomization::SetFailureMessage(const FText& NewText, ETextCommit::Type CommitType)
{
	SetMetaData(UE::AssetValidation::FailureMessage, !NewText.IsEmpty(), NewText.ToString());
}

EVisibility FPropertyValidationVariableDetailCustomization::GetValidateRecursiveVisibility() const
{
	if (const FProperty* Property = CachedProperty.Get())
	{
		const bool bCanApply = UE::AssetValidation::CanApplyMeta_ValidateRecursive(Property);
		return bCanApply ? EVisibility::Visible : EVisibility::Collapsed;
	}

	return EVisibility::Collapsed;
}

EVisibility FPropertyValidationVariableDetailCustomization::GetValidateKeyVisibility() const
{
	if (const FProperty* Property = CachedProperty.Get())
	{
		return UE::AssetValidation::CanApplyMeta_ValidateKey(Property) ? EVisibility::Visible : EVisibility::Collapsed;
	}

	return EVisibility::Collapsed;
}

EVisibility FPropertyValidationVariableDetailCustomization::GetValidateValueVisibility() const
{
	if (const FProperty* Property = CachedProperty.Get())
	{
		return UE::AssetValidation::CanApplyMeta_ValidateValue(Property) ? EVisibility::Visible : EVisibility::Collapsed;
	}
	
	return EVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE
