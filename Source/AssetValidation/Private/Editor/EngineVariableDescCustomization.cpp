#include "EngineVariableDescCustomization.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertySelector.h"
#include "PropertyValidationSettings.h"

void FEngineVariableDescCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// cache customized object
	TArray<UObject*> Objects;
	StructPropertyHandle->GetOuterObjects(Objects);
	CustomizedObject = Objects[0];
	
	StructHandle = StructPropertyHandle.ToSharedPtr();
	PropertyPathHandle = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEngineVariableDescription, PropertyPath));
	
	HeaderRow
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
	FEngineVariableDescription PropertyDesc = GetPropertyDescription();
	if (const FProperty* Property = PropertyDesc.GetProperty())
	{
		ChildBuilder.AddProperty(StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEngineVariableDescription, bValidate)).ToSharedRef());
		ChildBuilder.AddProperty(StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEngineVariableDescription, bValidateRecursive)).ToSharedRef());
		ChildBuilder.AddProperty(StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEngineVariableDescription, FailureMessage)).ToSharedRef());
	}
	// @todo: remove
	ChildBuilder.AddProperty(StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FEngineVariableDescription, Struct)).ToSharedRef());
}

void FEngineVariableDescCustomization::HandlePropertyChanged(TFieldPath<FProperty> NewPath)
{
	PropertyPathHandle->SetValue(NewPath.Get(GetOwningStruct()));
}

TFieldPath<FProperty> FEngineVariableDescCustomization::GetPropertyPath() const
{
	return GetPropertyDescription().PropertyPath;
}

UStruct* FEngineVariableDescCustomization::GetOwningStruct() const
{
	return GetPropertyDescription().Struct;
}

FEngineVariableDescription FEngineVariableDescCustomization::GetPropertyDescription() const
{
	FEngineVariableDescription* PropertyDescription = reinterpret_cast<FEngineVariableDescription*>(StructHandle->GetValueBaseAddress(reinterpret_cast<uint8*>(CustomizedObject.Get())));
	check(PropertyDescription);
	
	return *PropertyDescription;
}
