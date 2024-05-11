#include "UserDefinedStructValidationTabLayout.h"

#include "AssetValidationDefines.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyValidationSettings.h"
#include "Engine/UserDefinedStruct.h"
#include "PropertyValidators/PropertyValidation.h"

bool FPropertyValidationDetailsBuilder::FCustomizationTarget::HandleIsMetaVisible(const FName& MetaKey) const
{
	if (const FProperty* Property = WeakProperty.Get())
	{
		if (Property->IsA<FMapProperty>() && MetaKey == UE::AssetValidation::Validate)
		{
			// don't should all "Validate", "ValidateKey" and "ValidateValue" for map properties
			return false;
		}
		return UE::AssetValidation::CanApplyMeta(Property, MetaKey);
	}

	return false;
}

bool FPropertyValidationDetailsBuilder::FCustomizationTarget::HandleIsMetaEditable(FName MetaKey) const
{
	if (FProperty* Property = WeakProperty.Get())
	{
		if (MetaKey == UE::AssetValidation::FailureMessage)
		{
			if (FPropertyExternalValidationData* PropertyData = Customization.GetPropertyData(Property))
			{
				return PropertyData->HasMetaData(UE::AssetValidation::Validate) || PropertyData->HasMetaData(UE::AssetValidation::ValidateKey) || PropertyData->HasMetaData(UE::AssetValidation::ValidateValue);
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
		if (FPropertyExternalValidationData* PropertyData = Customization.GetPropertyData(WeakProperty.Get()))
		{
			if (PropertyData->HasMetaData(MetaKey))
			{
				OutValue = PropertyData->GetMetaData(MetaKey);
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
		if (FPropertyExternalValidationData* PropertyData = Customization.GetPropertyData(WeakProperty.Get()))
		{
			if (NewMetaState)
			{
				PropertyData->SetMetaData(MetaKey, MetaValue);
			}
			else
			{
				PropertyData->RemoveMetaData(MetaKey);
			}
		}
	}
}

void FUserDefinedStructValidationTabLayout::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailLayout.GetObjectsBeingCustomized(Objects);
	
	if (Objects.Num() == 0)
	{
		return;
	}

	UserDefinedStruct = CastChecked<UUserDefinedStruct>(Objects[0].Get());
	
	IDetailCategoryBuilder& Category = DetailLayout.EditCategory("Validation",
	NSLOCTEXT("AssetValidation", "ValidationDetailsCategory", "Validation"), ECategoryPriority::Default).InitiallyCollapsed(false);

	for (const FProperty* Property: TFieldRange<FProperty>{UserDefinedStruct.Get(), EFieldIterationFlags::Default})
	{
		if (UE::AssetValidation::CanValidatePropertyValue(Property) || UE::AssetValidation::CanValidatePropertyRecursively(Property))
		{
			TSharedPtr<IPropertyHandle> PropertyHandle = DetailLayout.AddStructurePropertyData(StructScope, Property->GetFName());
			TSharedRef<IDetailCustomNodeBuilder> PropertyBuilder = MakeShared<FPropertyValidationDetailsBuilder>(UserDefinedStruct.Get(), PropertyHandle);
			Category.AddCustomBuilder(PropertyBuilder);
		}
	}
}

FUserDefinedStructValidationTabLayout::~FUserDefinedStructValidationTabLayout()
{
	UE_LOG(LogAssetValidation, Verbose, TEXT("FUserDefinedStructValidationDetails has been destroyed."));
}

FPropertyValidationDetailsBuilder::FPropertyValidationDetailsBuilder(UUserDefinedStruct* InEditedStruct, TSharedPtr<IPropertyHandle> InPropertyHandle)
	: UserDefinedStruct(InEditedStruct)
	, PropertyHandle(InPropertyHandle)
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

FPropertyExternalValidationData* FPropertyValidationDetailsBuilder::GetPropertyData(FProperty* Property) const
{
	if (!UserDefinedStruct.IsValid())
	{
		return nullptr;
	}
	
	auto Settings = UPropertyValidationSettings::GetMutable();
	FStructExternalValidationData& StructData = Settings->UserDefinedStructs.FindOrAdd(GetEditedStructPath());

	FPropertyExternalValidationData* FoundData = StructData.Properties.FindByPredicate([Property](const FPropertyExternalValidationData& PropertyData)
	{
		return PropertyData.GetProperty() == Property;
	});
	if (FoundData == nullptr)
	{
		FoundData = &StructData.Properties.Add_GetRef(FPropertyExternalValidationData{UserDefinedStruct.Get(), Property});
	}

	return FoundData;
}

FSoftObjectPath FPropertyValidationDetailsBuilder::GetEditedStructPath() const
{
	return TSoftObjectPtr<UUserDefinedStruct>{UserDefinedStruct.Get()}.ToSoftObjectPath();
}

