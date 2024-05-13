#pragma once

#include "CoreMinimal.h"
#include "Kismet2/StructureEditorUtils.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class FBlueprintEditor;

namespace UE::AssetValidation
{
/**
 * Validation tab summoner for blueprint editor
 */
class FValidationTabSummoner: public FWorkflowTabFactory
{
public:
	FValidationTabSummoner(TSharedPtr<FBlueprintEditor> InBlueprintEditor);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;

protected:
	TWeakPtr<FBlueprintEditor> BlueprintEditor;
};

class SBlueprintEditorValidationTab: public SCompoundWidget, public FNotifyHook
{
public:
	SLATE_BEGIN_ARGS(SBlueprintEditorValidationTab)
	{}
	SLATE_ARGUMENT(TWeakPtr<FBlueprintEditor>, BlueprintEditor)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);
	
	//~Begin NotifyHook interface
	virtual void NotifyPreChange(FProperty* PropertyAboutToChange) override;
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged) override;
	//~EndNotifyHook interface
	
protected:
	/** owning blueprint editor */
	TWeakPtr<FBlueprintEditor> BlueprintEditor;
	/** owning blueprint */
	TWeakObjectPtr<UBlueprint> Blueprint;
	/** Property viewing widget */
	TSharedPtr<IDetailsView> DetailsView;
};
	
	
/**
 * Validation tab used for UDS editor
 */
class SUserDefinedStructValidationTab: public SCompoundWidget, public FStructureEditorUtils::INotifyOnStructChanged
{
public:
	SLATE_BEGIN_ARGS(SUserDefinedStructValidationTab)
	{}
	SLATE_ARGUMENT(UUserDefinedStruct*, Struct)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args);

	//~Begin INotifyOnStructChanged
	virtual void PreChange(const class UUserDefinedStruct* Struct, FStructureEditorUtils::EStructureEditorChangeInfo Info) override;
	virtual void PostChange(const class UUserDefinedStruct* Struct, FStructureEditorUtils::EStructureEditorChangeInfo Info) override;
	//~End INotifyOnStructChanged
private:
	/** Struct that widget represents */
	TWeakObjectPtr<UUserDefinedStruct> UserDefinedStruct;
	/** Struct scope */
	TSharedPtr<FStructOnScope> StructScope;
	/** Details view */
	TSharedPtr<IDetailsView> DetailsView;
};

}

