#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

#include "BlueprintEditorCustomization.generated.h"

class FBlueprintEditor;

UCLASS()
class UBlueprintValidationView: public UObject
{
	GENERATED_BODY()
};

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
	

class SBlueprintEditorValidationTab: public SCompoundWidget, public FGCObject, public FNotifyHook
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

	//~Begin FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override
	{
		return TEXT("SBlueprintEditorValidationTab");
	}
	//~End FGCObject interface
protected:

	void UpdateBlueprintView(bool bForceRefresh = false);
	void OnBlueprintChanged(UBlueprint* Blueprint);
	
	/** owning blueprint editor */
	TWeakPtr<FBlueprintEditor> BlueprintEditor;
	/** view object spawned by this tab */
	TWeakObjectPtr<UObject> BlueprintView;
	/** Property viewing widget */
	TSharedPtr<IDetailsView> DetailsView;
};


class FBlueprintEditorValidationTabLayout: public IDetailCustomization
{
	using ThisClass = FBlueprintEditorValidationTabLayout;
public:
	FBlueprintEditorValidationTabLayout(TWeakPtr<FBlueprintEditor> InBlueprintEditor)
		: BlueprintEditor(InBlueprintEditor)
	{}
	
	static TSharedRef<IDetailCustomization> MakeInstance(TWeakPtr<FBlueprintEditor> InBlueprintEditor)
	{
		return MakeShared<ThisClass>(InBlueprintEditor);
	}

	virtual void CustomizeDetails(class IDetailLayoutBuilder& DetailLayout) override;
	
protected:
	TWeakPtr<FBlueprintEditor> BlueprintEditor;
};

