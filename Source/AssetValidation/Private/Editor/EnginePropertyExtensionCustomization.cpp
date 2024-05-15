#include "EnginePropertyExtensionCustomization.h"

#include "AssetValidationDefines.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "SPropertySelector.h"
#include "PropertyValidationSettings.h"
#include "BlueprintVariableCustomization.h"
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
				const FEnginePropertyExtension& PropertyDesc = Shared->GetExternalPropertyData();
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
		const FEnginePropertyExtension& PropertyDesc = Customization.Pin()->GetExternalPropertyData();
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
	
		FEnginePropertyExtension& PropertyDesc = Shared->GetExternalPropertyData();
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

	using namespace UE::AssetValidation;

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
}

FEnginePropertyExtensionCustomization::~FEnginePropertyExtensionCustomization()
{
	UE_LOG(LogAssetValidation, Verbose, TEXT("FEngineVariableDescCustomization has been destroyed"));
}

void FEnginePropertyExtensionCustomization::HandlePropertyChanged(TFieldPath<FProperty> NewPath)
{
	// notify pre change to a struct value
	StructHandle->NotifyPreChange();

	FEnginePropertyExtension& PropertyData = GetExternalPropertyData();
	PropertyData.PropertyPath = NewPath.Get(PropertyData.Struct);
	PropertyData.MetaDataMap.Empty();

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
	return GetExternalPropertyData().GetProperty();
}

TFieldPath<FProperty> FEnginePropertyExtensionCustomization::GetPropertyPath() const
{
	return GetExternalPropertyData().PropertyPath;
}

UStruct* FEnginePropertyExtensionCustomization::GetOwningStruct() const
{
	return GetExternalPropertyData().Struct;
}

FEnginePropertyExtension& FEnginePropertyExtensionCustomization::GetExternalPropertyData() const
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