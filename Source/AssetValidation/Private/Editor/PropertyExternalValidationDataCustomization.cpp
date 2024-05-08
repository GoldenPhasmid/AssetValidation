#include "PropertyExternalValidationDataCustomization.h"

#include "AssetValidationDefines.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "SPropertySelector.h"
#include "PropertyValidationSettings.h"
#include "PropertyValidationVariableDetailCustomization.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

bool FPropertyExternalValidationDataCustomization::FCustomizationTarget::HandleIsMetaVisible(const FName& MetaKey) const
{
	if (Customization.IsValid())
	{
		if (const FProperty* Property = Customization.Pin()->GetProperty())
		{
			if (Property->IsA<FMapProperty>() && MetaKey == UE::AssetValidation::Validate)
			{
				// don't should all "Validate", "ValidateKey" and "ValidateValue" for map properties
				return false;
			}
			return UE::AssetValidation::CanApplyMeta(Property, MetaKey);
		}
	}
	
	return false;
}

bool FPropertyExternalValidationDataCustomization::FCustomizationTarget::HandleIsMetaEditable(FName MetaKey) const
{
	if (Customization.IsValid())
	{
		auto Shared = Customization.Pin();
		if (FProperty* Property = Shared->GetProperty())
		{
			if (MetaKey == UE::AssetValidation::FailureMessage)
			{
				// enable editing FailureMessage only if value validation meta is already set
				const FPropertyExternalValidationData& PropertyDesc = Shared->GetExternalPropertyData();
				return PropertyDesc.HasMetaData(UE::AssetValidation::Validate) || PropertyDesc.HasMetaData(UE::AssetValidation::ValidateKey) || PropertyDesc.HasMetaData(UE::AssetValidation::ValidateValue);
			}

			return true;
		}
	}

	return false;
}

bool FPropertyExternalValidationDataCustomization::FCustomizationTarget::HandleGetMetaState(const FName& MetaKey, FString& OutValue) const
{
	if (Customization.IsValid())
	{
		const FPropertyExternalValidationData& PropertyDesc = Customization.Pin()->GetExternalPropertyData();
		if (PropertyDesc.HasMetaData(MetaKey))
		{
			OutValue = PropertyDesc.GetMetaData(MetaKey);
			return true;
		}
	}
	
	return false;
}

void FPropertyExternalValidationDataCustomization::FCustomizationTarget::HandleMetaStateChanged(bool NewMetaState, const FName& MetaKey, FString MetaValue)
{
	if (Customization.IsValid())
	{
		auto Shared = Customization.Pin();
		Shared->StructHandle->NotifyPreChange();
		
		FPropertyExternalValidationData& PropertyDesc = Shared->GetExternalPropertyData();
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

void FPropertyExternalValidationDataCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// cache property utilities to update children layout when property value changes
	PropertyUtilities = CustomizationUtils.GetPropertyUtilities();
	
	// cache customized object
	TArray<UObject*> Objects;
	StructPropertyHandle->GetOuterObjects(Objects);
	CustomizedObject = Objects[0];
	
	StructHandle = StructPropertyHandle.ToSharedPtr();
	PropertyPathHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPropertyExternalValidationData, PropertyPath));
	PropertyPathHandle->MarkHiddenByCustomization();

	MetaDataMapHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPropertyExternalValidationData, MetaDataMap));
	MetaDataMapHandle->MarkHiddenByCustomization();
	
	StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FPropertyExternalValidationData, Struct))
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

void FPropertyExternalValidationDataCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
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

FPropertyExternalValidationDataCustomization::~FPropertyExternalValidationDataCustomization()
{
	UE_LOG(LogAssetValidation, Verbose, TEXT("FEngineVariableDescCustomization has been destroyed"));
}

void FPropertyExternalValidationDataCustomization::HandlePropertyChanged(TFieldPath<FProperty> NewPath)
{
	// notify pre change to a struct value
	StructHandle->NotifyPreChange();
	
	FPropertyExternalValidationData& PropertyData = GetExternalPropertyData();
	PropertyData.PropertyPath = NewPath.Get(PropertyData.Struct);
	PropertyData.MetaDataMap.Empty();
	
	// notify post change to a struct value
	StructHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	
	if (PropertyUtilities.IsValid())
	{
		// notify UI to refresh itself
		PropertyUtilities->RequestRefresh();
	}
}

FProperty* FPropertyExternalValidationDataCustomization::GetProperty() const
{
	return GetExternalPropertyData().GetProperty();
}

TFieldPath<FProperty> FPropertyExternalValidationDataCustomization::GetPropertyPath() const
{
	return GetExternalPropertyData().PropertyPath;
}

UStruct* FPropertyExternalValidationDataCustomization::GetOwningStruct() const
{
	return GetExternalPropertyData().Struct;
}

FPropertyExternalValidationData& FPropertyExternalValidationDataCustomization::GetExternalPropertyData() const
{
	if (uint8* StructValue = StructHandle->GetValueBaseAddress(reinterpret_cast<uint8*>(CustomizedObject.Get())))
	{
		return *reinterpret_cast<FPropertyExternalValidationData*>(StructValue);
	}

	static FPropertyExternalValidationData InvalidDesc;
	UE_LOG(LogAssetValidation, Error, TEXT("FEngineVariableDescCustomization: Failed to get struct value"));
	return InvalidDesc;
}

#undef LOCTEXT_NAMESPACE