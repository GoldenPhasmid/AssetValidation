#include "EngineVariableDescCustomization.h"

#include "AssetValidationModule.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"
#include "PropertySelector.h"
#include "PropertyValidationSettings.h"
#include "PropertyValidationVariableDetailCustomization.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

bool FEngineVariableDescCustomization::FCustomizationTarget::HandleIsMetaVisible(const FName& MetaKey) const
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

bool FEngineVariableDescCustomization::FCustomizationTarget::HandleIsMetaEditable(FName MetaKey) const
{
	return Customization.IsValid() && Customization.Pin()->GetProperty() != nullptr;
}

bool FEngineVariableDescCustomization::FCustomizationTarget::HandleGetMetaState(const FName& MetaKey, FString& OutValue) const
{
	if (Customization.IsValid())
	{
		const FEngineVariableDescription& PropertyDesc = Customization.Pin()->GetPropertyDescription();
		if (PropertyDesc.HasMetaData(MetaKey))
		{
			OutValue = PropertyDesc.GetMetaData(MetaKey);
			return true;
		}
	}
	
	return false;
}

void FEngineVariableDescCustomization::FCustomizationTarget::HandleMetaStateChanged(bool NewMetaState, const FName& MetaKey, FString MetaValue)
{
	if (Customization.IsValid())
	{
		FEngineVariableDescription& PropertyDesc = Customization.Pin()->GetPropertyDescription();
		PropertyDesc.SetMetaData(MetaKey, MetaValue);
	}
}

void FEngineVariableDescCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// cache property utilities to update children layout when property value changes
	PropertyUtilities = CustomizationUtils.GetPropertyUtilities();
	
	// cache customized object
	TArray<UObject*> Objects;
	StructPropertyHandle->GetOuterObjects(Objects);
	CustomizedObject = Objects[0];
	
	StructHandle = StructPropertyHandle.ToSharedPtr();
	PropertyPathHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEngineVariableDescription, PropertyPath));
	PropertyPathHandle->MarkHiddenByCustomization();

	MetaDataMapHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEngineVariableDescription, MetaDataMap));
	MetaDataMapHandle->MarkHiddenByCustomization();
	
	StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEngineVariableDescription, Struct))
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

void FEngineVariableDescCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
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

FEngineVariableDescCustomization::~FEngineVariableDescCustomization()
{
	UE_LOG(LogAssetValidation, Verbose, TEXT("FEngineVariableDescCustomization has been destroyed"));
}

void FEngineVariableDescCustomization::HandlePropertyChanged(TFieldPath<FProperty> NewPath)
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

FProperty* FEngineVariableDescCustomization::GetProperty() const
{
	return GetPropertyDescription().GetProperty();
}

TFieldPath<FProperty> FEngineVariableDescCustomization::GetPropertyPath() const
{
	return GetPropertyDescription().PropertyPath;
}

UStruct* FEngineVariableDescCustomization::GetOwningStruct() const
{
	return GetPropertyDescription().Struct;
}

FEngineVariableDescription& FEngineVariableDescCustomization::GetPropertyDescription() const
{
	if (uint8* StructValue = StructHandle->GetValueBaseAddress(reinterpret_cast<uint8*>(CustomizedObject.Get())))
	{
		return *reinterpret_cast<FEngineVariableDescription*>(StructValue);
	}

	static FEngineVariableDescription InvalidDesc;
	UE_LOG(LogAssetValidation, Error, TEXT("FEngineVariableDescCustomization: Failed to get struct value"));
	return InvalidDesc;
}

#undef LOCTEXT_NAMESPACE