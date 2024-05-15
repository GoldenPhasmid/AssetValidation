#pragma once

#include "CoreMinimal.h"
#include "Editor/CustomizationTarget.h"

struct FEnginePropertyExtension;
namespace UE::AssetValidation
{
	class SPropertySelector;
}

namespace UE::AssetValidation
{
	
class FEnginePropertyExtensionCustomization: public IPropertyTypeCustomization
{
	using ThisClass = FEnginePropertyExtensionCustomization;
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShared<ThisClass>();
	}
	
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual ~FEnginePropertyExtensionCustomization() override;

private:

	struct FCustomizationTarget: public UE::AssetValidation::ICustomizationTarget
	{
	public:
		FCustomizationTarget(FEnginePropertyExtensionCustomization& InCustomization)
			: Customization(StaticCastWeakPtr<FEnginePropertyExtensionCustomization>(InCustomization.AsWeak()))
		{}
		
		//~Begin ICustomizationTarget interface
		virtual bool HandleIsMetaVisible(const FName& MetaKey) const override;
		virtual bool HandleIsMetaEditable(FName MetaKey) const override;
		virtual bool HandleGetMetaState(const FName& MetaKey, FString& OutValue) const override;
		virtual void HandleMetaStateChanged(bool NewMetaState, const FName& MetaKey, FString MetaValue = {}) override;
		//~End ICustomizationTarget interface

		TWeakPtr<FEnginePropertyExtensionCustomization> Customization;
	};
	
	void HandlePropertyChanged(TFieldPath<FProperty> NewPath);

	/** @return property from customized struct */
	FProperty* GetProperty() const;
	/** @return property path from customized struct */
	TFieldPath<FProperty> GetPropertyPath() const;
	/** @return struct that owns the property from customized struct */
	UStruct* GetOwningStruct() const;

	FEnginePropertyExtension& GetExternalPropertyData() const;

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
	TSharedPtr<UE::AssetValidation::SPropertySelector> PropertySelector;
};

} // UE::AssetValidation

