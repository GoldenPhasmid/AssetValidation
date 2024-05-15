#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "IDetailCustomNodeBuilder.h"
#include "Editor/CustomizationTarget.h"

class FBlueprintEditor;

namespace UE::AssetValidation
{
	class ICustomizationTarget;
}

class FProperty;
class IBlueprintEditor;
class UBlueprint;
class UPropertyValidatorSubsystem;

namespace UE::AssetValidation
{
	
class FBlueprintVariableCustomization: public IDetailCustomization, public IDetailCustomNodeBuilder
{
	using ThisClass = FBlueprintVariableCustomization;
	struct FPrivateToken {};
public:

	static TSharedPtr<IDetailCustomization>		MakeInstance(TSharedPtr<IBlueprintEditor> InBlueprintEditor);
	static TSharedPtr<IDetailCustomNodeBuilder> MakeNodeBuilder(TSharedPtr<IBlueprintEditor> InBlueprintEditor, TSharedRef<IPropertyHandle> InPropertyHandle, FName CategoryName);

	// private constructors
	FBlueprintVariableCustomization(TSharedPtr<IBlueprintEditor> InBlueprintEditor, UBlueprint* InBlueprint, FPrivateToken = {})
		: BlueprintEditor(InBlueprintEditor)
		, Blueprint(InBlueprint)
	{}
	FBlueprintVariableCustomization(TSharedPtr<IBlueprintEditor> InBlueprintEditor, UBlueprint* InBlueprint, TSharedRef<IPropertyHandle> InPropertyHandle, FName InCategoryName, FPrivateToken = {})
		: BlueprintEditor(InBlueprintEditor)
		, Blueprint(InBlueprint)
		, PropertyHandle(InPropertyHandle)
		, CachedProperty(PropertyHandle->GetProperty())
		, CategoryName(InCategoryName)
	{}

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;

	// IDetailCustomNodeBuilder interface
	virtual ~FBlueprintVariableCustomization() override;
	virtual void GenerateHeaderRowContent( FDetailWidgetRow& NodeRow ) override;
	virtual void GenerateChildContent(IDetailChildrenBuilder& ChildrenBuilder) override;
	virtual void SetOnRebuildChildren(FSimpleDelegate InOnRebuildChildren) override { OnRebuildChildren = InOnRebuildChildren; } 
	virtual bool InitiallyCollapsed() const override { return false; }
	virtual bool RequiresTick() const override { return false; }
	virtual void Tick( float DeltaTime ) override {}
	virtual FName GetName() const override { return TEXT("BlueprintVariableCustomization"); }

private:

	void Initialize(UObject* EditedObject);

	struct FCustomizationTarget: public UE::AssetValidation::ICustomizationTarget
	{
	public:
		FCustomizationTarget(FBlueprintVariableCustomization& InCustomization)
			: Customization(StaticCastWeakPtr<FBlueprintVariableCustomization>(InCustomization.AsWeak()))
		{}
		//~Begin ICustomizationTarget interface
		virtual bool HandleIsMetaVisible(const FName& MetaKey) const override;
		virtual bool HandleIsMetaEditable(FName MetaKey) const override;
		virtual bool HandleGetMetaState(const FName& MetaKey, FString& OutValue) const override;
		virtual void HandleMetaStateChanged(bool NewMetaState, const FName& MetaKey, FString MetaValue = {}) override;
		//~End ICustomizationTarget interface

		TWeakPtr<FBlueprintVariableCustomization> Customization;
	};

	/**
	 * @return true if the displayed property is owned by the current Blueprint (and not parent or a local variable)
	 * @see FBlueprintVarActionDetails::IsVariableInBlueprint
	 */
	bool IsVariableInBlueprint() const;

	/**
	 * @return true if the displayed property is inherited by the current Blueprint
	 * @see FBlueprintVarActionDetails::IsVariableInheritedByBlueprint
	 */
	bool IsVariableInheritedByBlueprint() const;

	bool HasMetaData(const FName& MetaName) const;
	bool GetMetaData(const FName& MetaName, FString& OutValue) const;
	void SetMetaData(const FName& MetaName, bool bEnabled, const FString& MetaValue = {});
private:

	/** Customization target */
	TSharedPtr<FCustomizationTarget> CustomizationTarget;

	/** blueprint editor customization is called for */
	TWeakPtr<IBlueprintEditor>	BlueprintEditor;

	/** blueprint that is currently being edited */
	TWeakObjectPtr<UBlueprint>	Blueprint;

	/** blueprint of a currently displayed property. May differ from Blueprint, as property can be inherited from a parent blueprint */
	TWeakObjectPtr<UBlueprint>	OwnerBlueprint;
	
	/** property handle that is being displayed, can be null */
	TSharedPtr<IPropertyHandle> PropertyHandle;
	
	/** affected variable property that is being displayed */
	TWeakFieldPtr<FProperty>	CachedProperty;

	/** children rebuild delegate */
	FSimpleDelegate OnRebuildChildren;
	
	/** Category name */
	FName CategoryName = NAME_None;
};
	
} // UE::AssetValidation