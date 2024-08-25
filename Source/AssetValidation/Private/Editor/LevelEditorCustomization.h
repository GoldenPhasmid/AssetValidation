#pragma once

#include "CoreMinimal.h"
#include "Widgets/Docking/SDockTab.h"
#include "Misc/NotifyHook.h"

#include "LevelEditorCustomization.generated.h"

class UAVCommandletAction;
class UAVCommandletSearchFilter;
class IDetailsView;

/**
 * 
 */
class FAssetValidationToolkitManager: public TSharedFromThis<FAssetValidationToolkitManager> 
{
	using ThisClass = FAssetValidationToolkitManager;
public:
	FAssetValidationToolkitManager();
	~FAssetValidationToolkitManager();

private:
	void SummonTab();
	TSharedRef<SDockTab> CreateTabBody(const FSpawnTabArgs& TabArgs) const;

	FName TabName{TEXT("AssetValidationToolkit")};
};

/**
 * 
 */
class SAssetValidationToolkit: public SCompoundWidget, public FGCObject, public FNotifyHook
{
public:
	SLATE_BEGIN_ARGS(SAssetValidationToolkit)
	{}
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
		return GetWidgetClass().GetWidgetType().ToString();
	}
	//~End FGCObject interface

protected:

	/** view object spawned by this tab */
	TWeakObjectPtr<UObject> ToolkitView;
	/** Property viewing widget */
	TSharedPtr<IDetailsView> DetailsView;
};

UCLASS()
class UAssetValidationToolkitView: public UObject
{
	GENERATED_BODY()
public:

	UFUNCTION(CallInEditor, Category = "Actions")
	void CheckContent();

	UFUNCTION(CallInEditor, Category = "Actions")
	void CheckProjectSettings();

	UFUNCTION(CallInEditor, Category = "Commandlet")
	void ExecuteCommandlet();

	UPROPERTY(VisibleAnywhere, Category = "Commandlet")
	FString CommandletStatus = TEXT("OK");
	
	UPROPERTY(EditAnywhere, Instanced, Category = "Commandlet")
	TObjectPtr<UAVCommandletSearchFilter> SearchFilter;

	UPROPERTY(EditAnywhere, Instanced, Category = "Commandlet")
	TObjectPtr<UAVCommandletAction> Action;
};
