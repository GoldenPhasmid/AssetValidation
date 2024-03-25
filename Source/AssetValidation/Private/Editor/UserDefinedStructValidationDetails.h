#pragma once

#include "IDetailCustomization.h"
#include "IDetailCustomNodeBuilder.h"

class UUserDefinedStruct;
class IUserDefinedStructureEditor;
class FUDSValidationInfoLayout;

/**
 * Represents validation dock tab in UDS editor
 */
class FUserDefinedStructValidationDetails: public IDetailCustomization
{
public:

	FUserDefinedStructValidationDetails(TWeakPtr<IUserDefinedStructureEditor> InStructureEditor)
	: StructureEditor(InStructureEditor)
	{}

	static TSharedRef<IDetailCustomization> MakeInstance(TWeakPtr<IUserDefinedStructureEditor> StructureEditor)
	{
		return MakeShared<FUserDefinedStructValidationDetails>(StructureEditor);
	}
	
	virtual void CustomizeDetails(class IDetailLayoutBuilder& DetailLayout) override;

private:
	TWeakPtr<IUserDefinedStructureEditor> StructureEditor;
	TWeakObjectPtr<UUserDefinedStruct> UserDefinedStruct;
	TSharedPtr<FUDSValidationInfoLayout> Layout;
};

/**
 * Represents general layout of a validation panel in UDS editor
 */
class FUDSValidationInfoLayout: public IDetailCustomNodeBuilder
{
public:

	FUDSValidationInfoLayout(TWeakObjectPtr<UUserDefinedStruct> InStruct)
		: Struct(InStruct)
	{}

	//~Begin DetailCustomNodeBuilder interface
	virtual void SetOnRebuildChildren(FSimpleDelegate InOnRegenerateChildren) override
	{
		OnRegenerateChildren = InOnRegenerateChildren;
	}
	virtual void GenerateHeaderRowContent(FDetailWidgetRow& NodeRow) override {}
	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override {}
	virtual FName GetName() const override { return NAME_None; }
	virtual bool InitiallyCollapsed() const override { return false; }
	//~End DetailCustomNodeBuilder interface

private:
	TWeakObjectPtr<UUserDefinedStruct> Struct;
	FSimpleDelegate OnRegenerateChildren;
};

/**
 * Represents single property layout inside validation panel in UDS editor
 */
class FUDSVariableValidationInfoLayout: public IDetailCustomNodeBuilder
{ };
