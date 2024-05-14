#pragma once

#include "CustomizationTarget.h"
#include "IDetailCustomNodeBuilder.h"

class FPropertyValidationDetailsBuilder: public IDetailCustomNodeBuilder
{
public:
	FPropertyValidationDetailsBuilder(UObject* InEditedObject, TSharedRef<IPropertyHandle> InPropertyHandle, bool bInUseExternalMetaData);
	
	//~Begin IDetailCustomNodeBuilder interface
	virtual ~FPropertyValidationDetailsBuilder() override;
	virtual void GenerateHeaderRowContent( FDetailWidgetRow& NodeRow ) override;
	virtual void GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder ) override;
	virtual void SetOnRebuildChildren( FSimpleDelegate InOnRebuildChildren  ) override { OnRebuildChildren = InOnRebuildChildren; } 
	virtual bool InitiallyCollapsed() const override { return false; }
	virtual bool RequiresTick() const override { return false; }
	virtual void Tick( float DeltaTime ) override {}
	virtual FName GetName() const override { return TEXT("StructProperty"); }
	//~End IDetailCustomNodeBuilder interface

private:
	
	UE::AssetValidation::FMetaDataSource GetPropertyMetaData(FProperty* Property) const;
	
	struct FCustomizationTarget: public UE::AssetValidation::ICustomizationTarget
	{
	public:
		FCustomizationTarget(FPropertyValidationDetailsBuilder& InCustomization, FProperty* Property)
			: Customization(InCustomization)
			, WeakProperty(Property)
		{}
		//~Begin ICustomizationTarget interface
		virtual bool HandleIsMetaVisible(const FName& MetaKey) const override;
		virtual bool HandleIsMetaEditable(FName MetaKey) const override;
		virtual bool HandleGetMetaState(const FName& MetaKey, FString& OutValue) const override;
		virtual void HandleMetaStateChanged(bool NewMetaState, const FName& MetaKey, FString MetaValue = {}) override;
		//~End ICustomizationTarget interface
		
		FPropertyValidationDetailsBuilder& Customization;
		TWeakFieldPtr<FProperty> WeakProperty;
	};
	
	/** Customization target */
	TSharedPtr<FCustomizationTarget> CustomizationTarget;

	TWeakObjectPtr<UObject> EditedObject;
	TSharedRef<IPropertyHandle> PropertyHandle;
	bool bUseExternalMetaData = false;

	FSimpleDelegate OnRebuildChildren;
};

