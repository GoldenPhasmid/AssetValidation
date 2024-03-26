#pragma once

struct FEngineVariableDescription;
class SPropertySelector;

class FEngineVariableDescCustomization: public IPropertyTypeCustomization
{
	using ThisClass = FEngineVariableDescCustomization;
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShared<ThisClass>();
	}

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;

private:

	void HandlePropertyChanged(TFieldPath<FProperty> NewPath);

	TFieldPath<FProperty> GetPropertyPath() const;
	UStruct* GetOwningStruct() const;

	FEngineVariableDescription GetPropertyDescription() const;
	
	/** customized objects */
	TWeakObjectPtr<UObject> CustomizedObject;
	/** Handle to FEnginePropertyDescription struct */
	TSharedPtr<IPropertyHandle> StructHandle;
	/** Handle to property path */
	TSharedPtr<IPropertyHandle> PropertyPathHandle;
	/** Property table */
	TSharedPtr<IPropertyTable> PropertyTable;
	/** */
	TSharedPtr<SComboButton> ComboButton;
	/** */
	TSharedPtr<SPropertySelector> PropertySelector;
};
