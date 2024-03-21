#include "PropertyValidationVariableDetailCustomization.h"

#include "BlueprintEditorModule.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyValidatorSubsystem.h"
#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

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
	
	CachedValidationSubsystem = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();

	const FSlateFontInfo DetailFont = IDetailLayoutBuilder::GetDetailFont();
	IDetailCategoryBuilder& Category = DetailLayout.EditCategory("Validation", LOCTEXT("ValidationDetailsCategory", "Validation"), ECategoryPriority::Default).InitiallyCollapsed(false);
	
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
		.ToolTipText(LOCTEXT("FailureMessageToolTip", "Override message that is being displayed if validation for this properties has failed. For example: \'always set storage type for storage component\' instead of \'object property not set\'"))
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
	if (const FProperty* Property = CachedProperty.Get())
	{
		return Property->HasMetaData(MetaName);
	}

	return false;
}

void FPropertyValidationVariableDetailCustomization::SetMetaData(const FName& MetaName, bool bEnabled)
{
	if (FProperty* Property = CachedProperty.Get())
	{
		if (bEnabled)
		{
			Property->SetMetaData(MetaName, {});
		}
		else
		{
			Property->RemoveMetaData(MetaName);
		}
	}
}

bool FPropertyValidationVariableDetailCustomization::IsMetaEditingEnabled() const
{
	return IsVariableInBlueprint();
}

EVisibility FPropertyValidationVariableDetailCustomization::GetValidateVisibility() const
{
	if (CachedValidationSubsystem.IsValid())
	{
		if (const FProperty* Property = CachedProperty.Get())
		{
			if (Property->IsA<FMapProperty>())
			{
				// for map properties show ValidateKey/ValidateValue check boxes instead
				return EVisibility::Collapsed;
			}
			
			return CachedValidationSubsystem->HasValidatorForPropertyValue(Property) ? EVisibility::Visible : EVisibility::Collapsed;
		}
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
	if (const FProperty* Property = CachedProperty.Get())
	{
		return FText::FromString(Property->GetMetaData(UE::AssetValidation::FailureMessage));
	}

	return FText::GetEmpty();
}

void FPropertyValidationVariableDetailCustomization::SetFailureMessage(const FText& NewText, ETextCommit::Type CommitType)
{
	if (FProperty* Property = CachedProperty.Get())
	{
		if (NewText.IsEmpty())
		{
			Property->RemoveMetaData(UE::AssetValidation::FailureMessage);
		}
		else
		{
			Property->SetMetaData(UE::AssetValidation::FailureMessage, *NewText.ToString());
		}
	}
}

EVisibility FPropertyValidationVariableDetailCustomization::GetValidateRecursiveVisibility() const
{
	if (const FProperty* Property = CachedProperty.Get())
	{
		auto GetVisibility = [](const FProperty* Property)
		{
			return Property->IsA<FObjectPropertyBase>() ? EVisibility::Visible : EVisibility::Collapsed;	
		};
		
		if (GetVisibility(Property) == EVisibility::Visible)
		{
			// property is an object property
			return EVisibility::Visible; 
		}
		if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			// property is an array property of objects
			return GetVisibility(ArrayProperty->Inner);
		}
		if (const FSetProperty* SetProperty = CastField<FSetProperty>(Property))
		{
			// property is a set property of objects
			return GetVisibility(SetProperty->ElementProp);
		}
		if (const FMapProperty* MapProperty = CastField<FMapProperty>(Property))
		{
			// property is a map property with either key or value being an object
			if (GetVisibility(MapProperty->KeyProp) == EVisibility::Visible)
			{
				return EVisibility::Visible;
			}
			if (GetVisibility(MapProperty->ValueProp) == EVisibility::Visible)
			{
				return EVisibility::Visible;
			}

			return EVisibility::Collapsed;
		}
	}

	return EVisibility::Collapsed;
}

EVisibility FPropertyValidationVariableDetailCustomization::GetValidateKeyVisibility() const
{
	if (const FProperty* Property = CachedProperty.Get())
	{
		if (const FMapProperty* MapProperty = CastField<FMapProperty>(Property))
		{
			return CachedValidationSubsystem->HasValidatorForPropertyValue(MapProperty->KeyProp) ? EVisibility::Visible : EVisibility::Collapsed;
		}
	}

	return EVisibility::Collapsed;
}

EVisibility FPropertyValidationVariableDetailCustomization::GetValidateValueVisibility() const
{
	if (const FProperty* Property = CachedProperty.Get())
	{
		if (const FMapProperty* MapProperty = CastField<FMapProperty>(Property))
		{
			return CachedValidationSubsystem->HasValidatorForPropertyValue(MapProperty->ValueProp) ? EVisibility::Visible : EVisibility::Collapsed;
		}
	}
	
	return EVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE
