#pragma once

#include "CustomizationTarget.h"
#include "IDetailCustomization.h"
#include "IDetailCustomNodeBuilder.h"

class FBlueprintEditor;
class FSubobjectEditorTreeNode;

namespace UE::AssetValidation
{
	class ICustomizationTarget;
}

namespace UE::AssetValidation
{

class FBlueprintComponentCustomization: public IDetailCustomization, public IDetailCustomNodeBuilder
{
	using ThisClass = FBlueprintComponentCustomization;
	struct FPrivateToken {};
public:
	static TSharedPtr<IDetailCustomization>		MakeInstance(TSharedPtr<FBlueprintEditor> InBlueprintEditor, FOnGetDetailCustomizationInstance ChildDelegate);
	static TSharedPtr<IDetailCustomNodeBuilder> MakeNodeBuilder(TSharedPtr<FBlueprintEditor> InBlueprintEditor, TSharedPtr<IPropertyHandle> PropertyHandle);

	// private constructors
	FBlueprintComponentCustomization(TSharedPtr<FBlueprintEditor> InBlueprintEditor, FOnGetDetailCustomizationInstance InChildDelegate, FPrivateToken = {});
	FBlueprintComponentCustomization(TSharedPtr<FBlueprintEditor> InBlueprintEditor, TSharedPtr<IPropertyHandle> InPropertyHandle, FPrivateToken = {});
	
	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;

	// IDetailCustomNodeBuilder interface
	virtual ~FBlueprintComponentCustomization() override;
	virtual void GenerateHeaderRowContent( FDetailWidgetRow& NodeRow ) override;
	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override;
	virtual void SetOnRebuildChildren(FSimpleDelegate InOnRebuildChildren) override { OnRebuildChildren = InOnRebuildChildren; } 
	virtual bool InitiallyCollapsed() const override { return false; }
	virtual bool RequiresTick() const override { return false; }
	virtual void Tick( float DeltaTime ) override {}
	virtual FName GetName() const override { return TEXT("BlueprintComponentCustomization"); }
	
private:
	struct FCustomizationTarget: public UE::AssetValidation::ICustomizationTarget
	{
	public:
		FCustomizationTarget(FBlueprintComponentCustomization& InCustomization)
			: Customization(StaticCastWeakPtr<FBlueprintComponentCustomization>(InCustomization.AsWeak()))
		{}
		//~Begin ICustomizationTarget interface
		virtual bool HandleIsMetaVisible(const FName& MetaKey) const override;
		virtual bool HandleIsMetaEditable(FName MetaKey) const override;
		virtual bool HandleGetMetaState(const FName& MetaKey, FString& OutValue) const override;
		virtual void HandleMetaStateChanged(bool NewMetaState, const FName& MetaKey, FString MetaValue = {}) override;
		//~End ICustomizationTarget interface

		TWeakPtr<FBlueprintComponentCustomization> Customization;
	};
	
	bool IsInheritedComponent() const;
	bool HasMetaData(const FName& MetaName) const;
	bool GetMetaData(const FName& MetaName, FString& OutValue) const;
	void SetMetaData(const FName& MetaName, bool bEnabled, const FString& MetaValue = {});
	FName GetVariableName() const;

	/** blueprint editor customization is called for */
	TWeakPtr<FBlueprintEditor>			BlueprintEditor;
	/** blueprint that is currently being edited */
	TWeakObjectPtr<UBlueprint>			Blueprint;
	/** blueprint of a currently displayed property. May differ from Blueprint, as property can be inherited from a parent blueprint */
	TWeakObjectPtr<UBlueprint>			OwnerBlueprint;

	/** Customization target */
	TSharedPtr<FCustomizationTarget>	CustomizationTarget;
	/** Child customization delegate, can be unbound */
	FOnGetDetailCustomizationInstance	ChildCustomizationDelegate;
	/** child customization instance, can be null */
	TSharedPtr<IDetailCustomization>	ChildCustomization;
	/** children rebuild delegate */
	FSimpleDelegate OnRebuildChildren;
	
	/** The cached tree Node we're editing, used in details customization mode */
	TSharedPtr<FSubobjectEditorTreeNode> EditingNode;
	/** The cached property handle we're editing, used in node builder mode */
	TSharedPtr<IPropertyHandle> PropertyHandle;
};
	
} // UE::AssetValidation


