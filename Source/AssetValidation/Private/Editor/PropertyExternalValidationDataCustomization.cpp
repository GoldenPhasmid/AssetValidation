#include "PropertyExternalValidationDataCustomization.h"

#include "AssetValidationModule.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "PropertySelector.h"
#include "PropertyValidationSettings.h"
#include "PropertyValidationVariableDetailCustomization.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

bool FPropertyExternalValidationDataCustomization::FCustomizationTarget::HandleIsMetaVisible(const FName& MetaKey) const
{
	if (Customization.IsValid())
	{
		if (const FProperty* Property = Customization.Pin()->GetProperty())
		{
			return UE::AssetValidation::CanApplyMeta(Property, MetaKey);
		}
	}
	
	return false;
}

bool FPropertyExternalValidationDataCustomization::FCustomizationTarget::HandleIsMetaEditable(FName MetaKey) const
{
	return Customization.IsValid() && Customization.Pin()->GetProperty() != nullptr;
}

bool FPropertyExternalValidationDataCustomization::FCustomizationTarget::HandleGetMetaState(const FName& MetaKey, FString& OutValue) const
{
	if (Customization.IsValid())
	{
		const FPropertyExternalValidationData& PropertyDesc = Customization.Pin()->GetPropertyDescription();
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
		FPropertyExternalValidationData& PropertyDesc = Customization.Pin()->GetPropertyDescription();
		PropertyDesc.SetMetaData(MetaKey, MetaValue);
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
	// set property path value to a new path
	PropertyPathHandle->SetValue(NewPath.Get(GetOwningStruct()));
	// empty meta data map
	MetaDataMapHandle->AsMap()->Empty();
	
	if (PropertyUtilities.IsValid())
	{
		// notify UI to refresh itself
		PropertyUtilities->RequestRefresh();
	}
}

FProperty* FPropertyExternalValidationDataCustomization::GetProperty() const
{
	return GetPropertyDescription().GetProperty();
}

TFieldPath<FProperty> FPropertyExternalValidationDataCustomization::GetPropertyPath() const
{
	return GetPropertyDescription().PropertyPath;
}

UStruct* FPropertyExternalValidationDataCustomization::GetOwningStruct() const
{
	return GetPropertyDescription().Struct;
}

FPropertyExternalValidationData& FPropertyExternalValidationDataCustomization::GetPropertyDescription() const
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