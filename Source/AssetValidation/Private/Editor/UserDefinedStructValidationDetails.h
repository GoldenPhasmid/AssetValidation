#pragma once

#include "CustomizationTarget.h"
#include "IDetailCustomization.h"
#include "IDetailCustomNodeBuilder.h"
#include "Kismet2/StructureEditorUtils.h"

struct FPropertyExternalValidationData;
class UUserDefinedStruct;
class IUserDefinedStructureEditor;
class FUDSValidationInfoLayout;

class SValidationTabWidget: public SCompoundWidget, public FStructureEditorUtils::INotifyOnStructChanged
{
public:
	SLATE_BEGIN_ARGS(SValidationTabWidget)
	{}
	SLATE_ARGUMENT(UUserDefinedStruct*, Struct)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);
	
	virtual void PreChange(const class UUserDefinedStruct* Struct, FStructureEditorUtils::EStructureEditorChangeInfo Info) override;
	virtual void PostChange(const class UUserDefinedStruct* Struct, FStructureEditorUtils::EStructureEditorChangeInfo Info) override;

private:
	/** Struct that widget represents */
	TWeakObjectPtr<UUserDefinedStruct> UserDefinedStruct;
	/** Struct scope */
    TSharedPtr<FStructOnScope> StructScope;
	/** Details view */
	TSharedPtr<IDetailsView> DetailsView;
};

/**
 * Represents validation dock tab in UDS editor
 */
class FUserDefinedStructValidationDetails: public IDetailCustomization
{
	using ThisClass = FUserDefinedStructValidationDetails;
public:

	FUserDefinedStructValidationDetails(TSharedPtr<FStructOnScope> InStructScope)
		: StructScope(InStructScope)
	{}

	static TSharedRef<IDetailCustomization> MakeInstance(TSharedPtr<FStructOnScope> StructScope)
	{
		return MakeShared<FUserDefinedStructValidationDetails>(StructScope);
	}
	
	virtual void CustomizeDetails(class IDetailLayoutBuilder& DetailLayout) override;
	virtual ~FUserDefinedStructValidationDetails() override;
private:
	
	/** Customized user defined struct */
	TWeakObjectPtr<UUserDefinedStruct> UserDefinedStruct;
	/** Struct scope */
	TSharedPtr<FStructOnScope> StructScope;
};

class FStructPropertyDetailBuilder: public IDetailCustomNodeBuilder
{
public:
	FStructPropertyDetailBuilder(UUserDefinedStruct* InEditedStruct, TSharedPtr<IPropertyHandle> InPropertyHandle);
	
	//~Begin IDetailCustomNodeBuilder interface
	virtual ~FStructPropertyDetailBuilder() override;
	virtual void GenerateHeaderRowContent( FDetailWidgetRow& NodeRow ) override;
	virtual void GenerateChildContent( IDetailChildrenBuilder& ChildrenBuilder ) override;
	virtual void SetOnRebuildChildren( FSimpleDelegate InOnRebuildChildren  ) override { OnRebuildChildren = InOnRebuildChildren; } 
	virtual bool InitiallyCollapsed() const override { return false; }
	virtual bool RequiresTick() const override { return false; }
	virtual void Tick( float DeltaTime ) override {}
	virtual FName GetName() const override { return TEXT("StructProperty"); }
	//~End IDetailCustomNodeBuilder interface

private:

	FSoftObjectPath GetEditedStructPath() const;
	FPropertyExternalValidationData* GetPropertyData(FProperty* Property) const;
	
	struct FCustomizationTarget: public UE::AssetValidation::ICustomizationTarget
	{
	public:
		FCustomizationTarget(FStructPropertyDetailBuilder& InCustomization, FProperty* Property)
			: Customization(InCustomization)
			, WeakProperty(Property)
		{}
		//~Begin ICustomizationTarget interface
		virtual bool HandleIsMetaVisible(const FName& MetaKey) const override;
		virtual bool HandleIsMetaEditable(FName MetaKey) const override;
		virtual bool HandleGetMetaState(const FName& MetaKey, FString& OutValue) const override;
		virtual void HandleMetaStateChanged(bool NewMetaState, const FName& MetaKey, FString MetaValue = {}) override;
		//~End ICustomizationTarget interface
		
		FStructPropertyDetailBuilder& Customization;
		TWeakFieldPtr<FProperty> WeakProperty;
	};
	
	/** Customization target */
	TSharedPtr<FCustomizationTarget> CustomizationTarget;

	TWeakObjectPtr<UUserDefinedStruct> UserDefinedStruct;
	TSharedPtr<IPropertyHandle> PropertyHandle;

	FSimpleDelegate OnRebuildChildren;
};
