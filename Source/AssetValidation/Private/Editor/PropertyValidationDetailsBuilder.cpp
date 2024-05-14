#include "PropertyValidationDetailsBuilder.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "MetaDataSource.h"
#include "PropertyValidators/PropertyValidation.h"
#include "PropertyValidationSettings.h"
#include "Engine/UserDefinedStruct.h"

bool FPropertyValidationDetailsBuilder::FCustomizationTarget::HandleIsMetaVisible(const FName& MetaKey) const
{
	if (WeakProperty.IsValid())
	{
		return IsPropertyMetaVisible(WeakProperty.Get(), MetaKey);
	}

	return false;
}

bool FPropertyValidationDetailsBuilder::FCustomizationTarget::HandleIsMetaEditable(FName MetaKey) const
{
	if (WeakProperty.IsValid())
	{
		if (MetaKey == UE::AssetValidation::FailureMessage)
		{
			if (UE::AssetValidation::FMetaDataSource MetaData = Customization.GetPropertyMetaData(WeakProperty.Get()); MetaData.IsValid())
			{
				return MetaData.HasMetaData(UE::AssetValidation::Validate) || MetaData.HasMetaData(UE::AssetValidation::ValidateKey) || MetaData.HasMetaData(UE::AssetValidation::ValidateValue);
			}
		}

		return true;
	}

	return false;
}

bool FPropertyValidationDetailsBuilder::FCustomizationTarget::HandleGetMetaState(const FName& MetaKey, FString& OutValue) const
{
	if (WeakProperty.IsValid())
	{
		if (UE::AssetValidation::FMetaDataSource MetaData = Customization.GetPropertyMetaData(WeakProperty.Get()); MetaData.IsValid())
		{
			if (MetaData.HasMetaData(MetaKey))
			{
				OutValue = MetaData.GetMetaData(MetaKey);
				return true;
			}
		}
	}

	return false;
}

void FPropertyValidationDetailsBuilder::FCustomizationTarget::HandleMetaStateChanged(bool NewMetaState, const FName& MetaKey, FString MetaValue)
{
	if (WeakProperty.IsValid())
	{
		if (UE::AssetValidation::FMetaDataSource MetaData = Customization.GetPropertyMetaData(WeakProperty.Get()); MetaData.IsValid())
		{
			if (NewMetaState)
			{
				MetaData.SetMetaData(MetaKey, MetaValue);
			}
			else
			{
				MetaData.RemoveMetaData(MetaKey);
			}
			
			// @todo: this doesn't do anything
			if (Customization.bUseExternalMetaData)
			{
				FPropertyExternalValidationData PropertyData = MetaData.GetExternalData();
			}
		}
	}
}

FPropertyValidationDetailsBuilder::FPropertyValidationDetailsBuilder(UObject* InEditedObject, TSharedRef<IPropertyHandle> InPropertyHandle, bool bInUseExternalMetaData)
	: EditedObject(InEditedObject)
	, PropertyHandle(InPropertyHandle)
	, bUseExternalMetaData(bInUseExternalMetaData)
{
	
}

FPropertyValidationDetailsBuilder::~FPropertyValidationDetailsBuilder()
{
	CustomizationTarget.Reset();
}

void FPropertyValidationDetailsBuilder::GenerateHeaderRowContent(FDetailWidgetRow& NodeRow)
{
	NodeRow
	.ShouldAutoExpand(true)
	.WholeRowContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	];
}

void FPropertyValidationDetailsBuilder::GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder)
{
	CustomizationTarget = MakeShared<FCustomizationTarget>(*this, PropertyHandle->GetProperty());
	CustomizationTarget->CustomizeForObject(CustomizationTarget, [&ChildrenBuilder](const FText& SearchString) -> FDetailWidgetRow&
	{
		return ChildrenBuilder.AddCustomRow(SearchString).ShouldAutoExpand(true);
	});
}

UE::AssetValidation::FMetaDataSource FPropertyValidationDetailsBuilder::GetPropertyMetaData(FProperty* Property) const
{
	UE::AssetValidation::FMetaDataSource MetaData{};
	if (EditedObject.IsValid())
	{
		if (!bUseExternalMetaData)
		{
			MetaData.SetProperty(Property);
		}
		else
		{
			UUserDefinedStruct* EditedStruct = CastChecked<UUserDefinedStruct>(EditedObject.Get());
			FPropertyExternalValidationData PropertyData = UPropertyValidationSettings::GetPropertyExternalValidationData(EditedStruct, Property);
			MetaData.SetExternalData(PropertyData);
		}
	}
	
	return MetaData;
}
