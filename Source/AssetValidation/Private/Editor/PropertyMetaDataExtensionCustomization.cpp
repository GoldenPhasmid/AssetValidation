#include "PropertyMetaDataExtensionCustomization.h"

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
	
bool FPropertyMetaDataExtensionCustomization::FCustomizationTarget::HandleIsMetaVisible(const FName& MetaKey) const
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

bool FPropertyMetaDataExtensionCustomization::FCustomizationTarget::HandleIsMetaEditable(FName MetaKey) const
{
	if (Customization.IsValid())
	{
		auto Shared = Customization.Pin();
		if (FProperty* Property = Shared->GetProperty())
		{
			if (MetaKey == UE::AssetValidation::FailureMessage)
			{
				// enable editing FailureMessage only if value validation meta is already set
				const FPropertyMetaDataExtension& PropertyDesc = Shared->GetPropertyExtension();
				return PropertyDesc.HasMetaData(UE::AssetValidation::Validate) || PropertyDesc.HasMetaData(UE::AssetValidation::ValidateKey) || PropertyDesc.HasMetaData(UE::AssetValidation::ValidateValue);
			}

			return true;
		}
	}

	return false;
}

bool FPropertyMetaDataExtensionCustomization::FCustomizationTarget::HandleGetMetaState(const FName& MetaKey, FString& OutValue) const
{
	if (Customization.IsValid())
	{
		const FPropertyMetaDataExtension& PropertyDesc = Customization.Pin()->GetPropertyExtension();
		if (PropertyDesc.HasMetaData(MetaKey))
		{
			OutValue = PropertyDesc.GetMetaData(MetaKey);
			return true;
		}
	}

	return false;
}

void FPropertyMetaDataExtensionCustomization::FCustomizationTarget::HandleMetaStateChanged(bool NewMetaState, const FName& MetaKey, FString MetaValue)
{
	if (Customization.IsValid())
	{
		auto Shared = Customization.Pin();
		Shared->StructHandle->NotifyPreChange();
	
		FPropertyMetaDataExtension& PropertyDesc = Shared->GetPropertyExtension();
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

void FPropertyMetaDataExtensionCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// cache property utilities to update children layout when property value changes
	PropertyUtilities = CustomizationUtils.GetPropertyUtilities();

	// cache customized object
	TArray<UObject*> Objects;
	StructPropertyHandle->GetOuterObjects(Objects);
	CustomizedObject = Objects[0];

	StructHandle = StructPropertyHandle.ToSharedPtr();
	PropertyPathHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPropertyMetaDataExtension, PropertyPath));
	PropertyPathHandle->MarkHiddenByCustomization();

	MetaDataMapHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPropertyMetaDataExtension, MetaDataMap));
	MetaDataMapHandle->MarkHiddenByCustomization();

	StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPropertyMetaDataExtension, Struct))
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

void FPropertyMetaDataExtensionCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	const FProperty* Property = GetProperty();
	if (Property == nullptr)
	{
		// nothing to customize
		return;
	}

	CustomizationTarget = MakeShared<FCustomizationTarget>(*this);
	CustomizationTarget->CustomizeForObject(CustomizationTarget, Property->GetDisplayNameText(),
		[&ChildBuilder](const FText& SearchString) -> FDetailWidgetRow&
	{
		return ChildBuilder.AddCustomRow(SearchString).ShouldAutoExpand(true);
	});
}

FPropertyMetaDataExtensionCustomization::~FPropertyMetaDataExtensionCustomization()
{
	UE_LOG(LogAssetValidation, Verbose, TEXT("FEngineVariableDescCustomization has been destroyed"));
}

void FPropertyMetaDataExtensionCustomization::HandlePropertyChanged(TFieldPath<FProperty> NewPath)
{
	// notify pre change to a struct value
	StructHandle->NotifyPreChange();

	FPropertyMetaDataExtension& PropertyExtension = GetPropertyExtension();
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

FProperty* FPropertyMetaDataExtensionCustomization::GetProperty() const
{
	return GetPropertyExtension().GetProperty();
}

TFieldPath<FProperty> FPropertyMetaDataExtensionCustomization::GetPropertyPath() const
{
	return GetPropertyExtension().PropertyPath;
}

UStruct* FPropertyMetaDataExtensionCustomization::GetOwningStruct() const
{
	return GetPropertyExtension().Struct;
}

FPropertyMetaDataExtension& FPropertyMetaDataExtensionCustomization::GetPropertyExtension() const
{
	if (uint8* StructValue = StructHandle->GetValueBaseAddress(reinterpret_cast<uint8*>(CustomizedObject.Get())))
	{
		return *reinterpret_cast<FPropertyMetaDataExtension*>(StructValue);
	}

	static FPropertyMetaDataExtension InvalidDesc;
	UE_LOG(LogAssetValidation, Error, TEXT("FEngineVariableDescCustomization: Failed to get struct value"));
	return InvalidDesc;
}
	
} // UE::AssetValidation

#undef LOCTEXT_NAMESPACE