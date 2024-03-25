#pragma once

class FEnginePropertyDescriptionCustomization: public IPropertyTypeCustomization
{
	using ThisClass = FEnginePropertyDescriptionCustomization;
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShared<ThisClass>();
	}

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:

	TSharedRef<SWidget> ConstructPropertyTree() const;
	FText GetPropertyName() const;

	/** customized objects */
	TWeakObjectPtr<UObject> CustomizedObject;
	/** struct property handle */
	TSharedPtr<IPropertyHandle> StructHandle;
	/** Property table */
	TSharedPtr<IPropertyTable> PropertyTable;
	/** */
	TSharedPtr<SComboButton> ComboButton;
};
