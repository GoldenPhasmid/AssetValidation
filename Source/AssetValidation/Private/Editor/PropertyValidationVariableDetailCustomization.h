#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class FProperty;
class IBlueprintEditor;
class UBlueprint;
class UPropertyValidatorSubsystem;

class FPropertyValidationVariableDetailCustomization: public IDetailCustomization
{
	using ThisClass = FPropertyValidationVariableDetailCustomization;
public:

	static TSharedPtr<IDetailCustomization> MakeInstance(TSharedPtr<IBlueprintEditor> InBlueprintEditor);

	FPropertyValidationVariableDetailCustomization(TSharedPtr<IBlueprintEditor> InBlueprintEditor, UBlueprint* InBlueprint)
		: BlueprintEditor(InBlueprintEditor)
		, Blueprint(InBlueprint)
	{}

	// IDetailCustomization interface
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;

private:

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
	void SetMetaData(const FName& MetaName, bool bEnabled, const FString& MetaValue = {});
private:
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
