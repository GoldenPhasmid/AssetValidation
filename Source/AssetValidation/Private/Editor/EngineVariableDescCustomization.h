#pragma once

#include "CoreMinimal.h"
#include "Editor/CustomizationTarget.h"

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
	virtual ~FEngineVariableDescCustomization() override;

private:

	struct FCustomizationTarget: public UE::AssetValidation::ICustomizationTarget
	{
	public:
		FCustomizationTarget(FEngineVariableDescCustomization& InCustomization)
			: Customization(StaticCastWeakPtr<FEngineVariableDescCustomization>(InCustomization.AsWeak()))
		{}
		
		//~Begin ICustomizationTarget interface
		virtual bool HandleIsMetaVisible(const FName& MetaKey) const override;
		virtual bool HandleIsMetaEditable(FName MetaKey) const override;
		virtual bool HandleGetMetaState(const FName& MetaKey, FString& OutValue) const override;
		virtual void HandleMetaStateChanged(bool NewMetaState, const FName& MetaKey, FString MetaValue = {}) override;
		//~End ICustomizationTarget interface

		TWeakPtr<FEngineVariableDescCustomization> Customization;
	};
	
	void HandlePropertyChanged(TFieldPath<FProperty> NewPath);

	/** @return property from customized struct */
	FProperty* GetProperty() const;
	/** @return property path from customized struct */
	TFieldPath<FProperty> GetPropertyPath() const;
	/** @return struct that owns the property from customized struct */
	UStruct* GetOwningStruct() const;

	FEngineVariableDescription& GetPropertyDescription() const;

	/** */
	TSharedPtr<FCustomizationTarget> CustomizationTarget;
	/** */
	TSharedPtr<IPropertyUtilities> PropertyUtilities;
	/** customized objects */
	TWeakObjectPtr<UObject> CustomizedObject;
	/** Handle to FEnginePropertyDescription struct */
	TSharedPtr<IPropertyHandle> StructHandle;
	/** Handle to property path */
	TSharedPtr<IPropertyHandle> PropertyPathHandle;
	/* Handle to meta data map */
	TSharedPtr<IPropertyHandle> MetaDataMapHandle;
	/** */
	TSharedPtr<SComboButton> ComboButton;
	/** */
	TSharedPtr<SPropertySelector> PropertySelector;
};
