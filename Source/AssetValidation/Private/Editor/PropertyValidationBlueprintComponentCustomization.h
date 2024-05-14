#pragma once

#include "CustomizationTarget.h"
#include "IDetailCustomization.h"

class FBlueprintEditor;
class FSubobjectEditorTreeNode;

namespace UE::AssetValidation
{
	class ICustomizationTarget;
}

class FPropertyValidationBlueprintComponentCustomization: public IDetailCustomization
{
	using ThisClass = FPropertyValidationBlueprintComponentCustomization;
public:
	static TSharedPtr<IDetailCustomization> MakeInstance(TSharedPtr<FBlueprintEditor> InBlueprintEditor, FOnGetDetailCustomizationInstance ChildDelegate);

	FPropertyValidationBlueprintComponentCustomization(TSharedPtr<FBlueprintEditor> InBlueprintEditor, FOnGetDetailCustomizationInstance InChildDelegate);

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	
private:
	struct FCustomizationTarget: public UE::AssetValidation::ICustomizationTarget
	{
	public:
		FCustomizationTarget(FPropertyValidationBlueprintComponentCustomization& InCustomization)
			: Customization(StaticCastWeakPtr<FPropertyValidationBlueprintComponentCustomization>(InCustomization.AsWeak()))
		{}
		//~Begin ICustomizationTarget interface
		virtual bool HandleIsMetaVisible(const FName& MetaKey) const override;
		virtual bool HandleIsMetaEditable(FName MetaKey) const override;
		virtual bool HandleGetMetaState(const FName& MetaKey, FString& OutValue) const override;
		virtual void HandleMetaStateChanged(bool NewMetaState, const FName& MetaKey, FString MetaValue = {}) override;
		//~End ICustomizationTarget interface

		TWeakPtr<FPropertyValidationBlueprintComponentCustomization> Customization;
	};
	
	bool IsInheritedComponent() const;
	bool HasMetaData(const FName& MetaName) const;
	bool GetMetaData(const FName& MetaName, FString& OutValue) const;
	void SetMetaData(const FName& MetaName, bool bEnabled, const FString& MetaValue = {});

	/** blueprint editor customization is called for */
	TWeakPtr<FBlueprintEditor>			BlueprintEditor;
	/** blueprint that is currently being edited */
	TWeakObjectPtr<UBlueprint>			Blueprint;

	/** Customization target */
	TSharedPtr<FCustomizationTarget>	CustomizationTarget;
	/** Child customization delegate, can be unbound */
	FOnGetDetailCustomizationInstance	ChildCustomizationDelegate;
	/** child customization instance, can be null */
	TSharedPtr<IDetailCustomization>	ChildCustomization;
	
	/** The cached tree Node we're editing */
	TSharedPtr<FSubobjectEditorTreeNode> EditingNode;
};
