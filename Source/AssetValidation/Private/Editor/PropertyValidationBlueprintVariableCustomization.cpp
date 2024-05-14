#include "PropertyValidationBlueprintVariableCustomization.h"

#include "BlueprintEditorModule.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyValidatorSubsystem.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "PropertyValidators/PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

bool FPropertyValidationBlueprintVariableCustomization::FCustomizationTarget::HandleIsMetaVisible(const FName& MetaKey) const
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

bool FPropertyValidationBlueprintVariableCustomization::FCustomizationTarget::HandleIsMetaEditable(FName MetaKey) const
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

bool FPropertyValidationBlueprintVariableCustomization::FCustomizationTarget::HandleGetMetaState(const FName& MetaKey, FString& OutValue) const
{
	return Customization.IsValid() && Customization.Pin()->GetMetaData(MetaKey, OutValue);
}

void FPropertyValidationBlueprintVariableCustomization::FCustomizationTarget::HandleMetaStateChanged(bool NewMetaState, const FName& MetaKey, FString MetaValue)
{
	if (Customization.IsValid())
	{
		Customization.Pin()->SetMetaData(MetaKey, NewMetaState, MetaValue);
	}
}

TSharedPtr<IDetailCustomization> FPropertyValidationBlueprintVariableCustomization::MakeInstance(TSharedPtr<IBlueprintEditor> InBlueprintEditor)
{
	const TArray<UObject*>* Objects = InBlueprintEditor.IsValid() ? InBlueprintEditor->GetObjectsCurrentlyBeingEdited() : nullptr;
	if (Objects && Objects->Num() == 1)
	{
		if (UBlueprint* Blueprint = Cast<UBlueprint>((*Objects)[0]))
		{
			return MakeShared<ThisClass>(InBlueprintEditor, Blueprint);
		}
	}
	
	return nullptr;
}

void FPropertyValidationBlueprintVariableCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
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
		OwnerBlueprint = Cast<UBlueprint>(BlueprintClass->ClassGeneratedBy);
	}
	
	IDetailCategoryBuilder& Category = DetailLayout.EditCategory(
		"Validation",
		LOCTEXT("ValidationCategoryTitle", "Validation"),
		ECategoryPriority::Default).InitiallyCollapsed(false);

	CustomizationTarget = MakeShared<FCustomizationTarget>(*this);
	CustomizationTarget->CustomizeForObject(CustomizationTarget, [&Category](const FText& SearchString) -> FDetailWidgetRow&
	{
		return Category.AddCustomRow(SearchString).ShouldAutoExpand(true);
	});	
}

bool FPropertyValidationBlueprintVariableCustomization::IsVariableInBlueprint() const
{
	return Blueprint.Get() == OwnerBlueprint.Get();
}

bool FPropertyValidationBlueprintVariableCustomization::IsVariableInheritedByBlueprint() const
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

bool FPropertyValidationBlueprintVariableCustomization::HasMetaData(const FName& MetaName) const
{
	FString MetaValue{};
	return GetMetaData(MetaName, MetaValue);
}


bool FPropertyValidationBlueprintVariableCustomization::GetMetaData(const FName& MetaName, FString& OutValue) const
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

void FPropertyValidationBlueprintVariableCustomization::SetMetaData(const FName& MetaName, bool bEnabled, const FString& MetaValue)
{
	if (Blueprint.IsValid() && CachedProperty.IsValid())
	{
		if (bEnabled)
		{
			FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint.Get(), CachedProperty->GetFName(), nullptr, MetaName, MetaValue);
		}
		else
		{
			FBlueprintEditorUtils::RemoveBlueprintVariableMetaData(Blueprint.Get(), CachedProperty->GetFName(), nullptr, MetaName);
		}
	}
}

#undef LOCTEXT_NAMESPACE
