#include "EnginePropertyDescriptionCustomization.h"

#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyValidationSettings.h"

void FEnginePropertyDescriptionCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	StructHandle = StructPropertyHandle.ToSharedPtr();
	TSharedPtr<IPropertyHandle> PropertyPathHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEnginePropertyDescription, PropertyPath));

	HeaderRow
	.NameContent()
	[
		PropertyPathHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		PropertyPathHandle->CreatePropertyValueWidget()
#if 0
		SAssignNew(ComboButton, SComboButton)
		.IsEnabled(true)
		.OnGetMenuContent(this, &ThisClass::ConstructPropertyTree)
		.ButtonContent()
		[
			SNew(STextBlock)
			.Text(this, &ThisClass::GetPropertyName)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
#endif
	];
}

void FEnginePropertyDescriptionCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	TArray<UObject*> Objects;
	StructPropertyHandle->GetOuterObjects(Objects);
	CustomizedObject = Objects[0];
	
	FEnginePropertyDescription* PropertyDescription = reinterpret_cast<FEnginePropertyDescription*>(StructPropertyHandle->GetValueBaseAddress(reinterpret_cast<uint8*>(CustomizedObject.Get())));
	check(PropertyDescription);
	
	// if (const FProperty* Property = PropertyDescription->PropertyPath.Get(CustomizedObject->GetClass()))
	{
		ChildBuilder.AddProperty(StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEnginePropertyDescription, bValidate)).ToSharedRef());
		ChildBuilder.AddProperty(StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEnginePropertyDescription, bValidateRecursive)).ToSharedRef());
		ChildBuilder.AddProperty(StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEnginePropertyDescription, FailureMessage)).ToSharedRef());
	}
}

TSharedRef<SWidget> FEnginePropertyDescriptionCustomization::ConstructPropertyTree() const
{
	return SNew(STextBlock);
}

FText FEnginePropertyDescriptionCustomization::GetPropertyName() const
{
	return FText::GetEmpty();
}
