#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Editor/CustomizationTarget.h"
#include "PropertyValidators/PropertyValidation.h"

namespace UE::AssetValidation
{
	class ICustomizationTarget;
}

class FProperty;
class IBlueprintEditor;
class UBlueprint;
class UPropertyValidatorSubsystem;

class FPropertyValidationBlueprintVariableCustomization: public IDetailCustomization
{
	using ThisClass = FPropertyValidationBlueprintVariableCustomization;
public:

	static TSharedPtr<IDetailCustomization> MakeInstance(TSharedPtr<IBlueprintEditor> InBlueprintEditor);

	FPropertyValidationBlueprintVariableCustomization(TSharedPtr<IBlueprintEditor> InBlueprintEditor, UBlueprint* InBlueprint)
		: BlueprintEditor(InBlueprintEditor)
		, Blueprint(InBlueprint)
	{}
	
	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;

private:

	struct FCustomizationTarget: public UE::AssetValidation::ICustomizationTarget
	{
	public:
		FCustomizationTarget(FPropertyValidationBlueprintVariableCustomization& InCustomization)
			: Customization(StaticCastWeakPtr<FPropertyValidationBlueprintVariableCustomization>(InCustomization.AsWeak()))
		{}
		//~Begin ICustomizationTarget interface
		virtual bool HandleIsMetaVisible(const FName& MetaKey) const override;
		virtual bool HandleIsMetaEditable(FName MetaKey) const override;
		virtual bool HandleGetMetaState(const FName& MetaKey, FString& OutValue) const override;
		virtual void HandleMetaStateChanged(bool NewMetaState, const FName& MetaKey, FString MetaValue = {}) override;
		//~End ICustomizationTarget interface

		TWeakPtr<FPropertyValidationBlueprintVariableCustomization> Customization;
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

	/** @return return true if validation meta editing is enabled */
	bool IsMetaEditingEnabled() const;
	
	EVisibility GetValidateVisibility() const;
	ECheckBoxState GetValidateState() const;
	void SetValidateState(ECheckBoxState NewState);

	EVisibility GetValidateRecursiveVisibility() const;
	ECheckBoxState GetValidateRecursiveState() const;
	void SetValidateRecursiveState(ECheckBoxState NewState);
	
	EVisibility GetValidateKeyVisibility() const;
	ECheckBoxState GetValidateKeyState() const;
	void SetValidateKeyState(ECheckBoxState NewState);

	EVisibility GetValidateValueVisibility() const;
	ECheckBoxState GetValidateValueState() const;
	void SetValidateValueState(ECheckBoxState NewState);

	EVisibility IsFailureMessageVisible() const;
	bool IsFailureMessageEnabled() const;
	FText GetFailureMessage() const;
	void SetFailureMessage(const FText& NewText, ETextCommit::Type CommitType);

	bool HasMetaData(const FName& MetaName) const;
	bool GetMetaData(const FName& MetaName, FString& OutValue) const;
	void SetMetaData(const FName& MetaName, bool bEnabled, const FString& MetaValue = {});
private:

	/** Customization target */
	TSharedPtr<FCustomizationTarget> CustomizationTarget;

	/** blueprint editor customization is called for */
	TWeakPtr<IBlueprintEditor> BlueprintEditor;
	
	/** blueprint that is currently being edited */
	TWeakObjectPtr<UBlueprint> Blueprint;

	/** blueprint of a currently displayed property. May differ from Blueprint, as property can be inherited from a parent blueprint */
	TWeakObjectPtr<UBlueprint> PropertyBlueprint;

	/** affected variable property that is being displayed */
	TWeakFieldPtr<FProperty> CachedProperty;
	
	/** affected variable cached name */
	FName CachedVariableName = NAME_None;

	/** cached validation subsystem */
	TWeakObjectPtr<UPropertyValidatorSubsystem> CachedValidationSubsystem;
};
