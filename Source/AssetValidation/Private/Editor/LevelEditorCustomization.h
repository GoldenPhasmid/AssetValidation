#pragma once

#include "CoreMinimal.h"
#include "Widgets/Docking/SDockTab.h"
#include "Misc/NotifyHook.h"

#include "LevelEditorCustomization.generated.h"

class UAVCommandletAction;
class UAVCommandletSearchFilter;
class IDetailsView;

/**
 * Manager that handles register/unregister action for Asset Validation menu entries
 */
class FAssetValidationMenuExtensionManager: public TSharedFromThis<FAssetValidationMenuExtensionManager> 
{
	using ThisClass = FAssetValidationMenuExtensionManager;
public:
	FAssetValidationMenuExtensionManager();
	~FAssetValidationMenuExtensionManager();

private:
	void SummonTab();
	TSharedRef<SDockTab> CreateTabBody(const FSpawnTabArgs& TabArgs) const;

	FName TabName{TEXT("AssetValidationToolkit")};
};

/**
 * Window view for asset validation commandlet functionality
 * Uses UAssetValidationToolkit property view
 */
class SAssetValidationToolkitView: public SCompoundWidget, public FGCObject, public FNotifyHook
{
public:
	SLATE_BEGIN_ARGS(SAssetValidationToolkitView)
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

/**
 * Object that represents asset validation commandlet functionality in editor window
 * Viewed via SAssetValidationToolkit
 */
UCLASS()
class UAssetValidationToolkit: public UObject
{
	GENERATED_BODY()
public:

	virtual void PostInitProperties() override;

	UFUNCTION(CallInEditor, Category = "Actions")
	void CheckContent();

	UFUNCTION(CallInEditor, Category = "Actions")
	void CheckProjectSettings();

	UFUNCTION(CallInEditor, Category = "Commandlet")
	void ExecuteCommandlet();

	UPROPERTY(VisibleAnywhere, Category = "Commandlet")
	FString CommandletStatus;
	
	UPROPERTY(EditAnywhere, Instanced, Category = "Commandlet")
	TObjectPtr<UAVCommandletSearchFilter> SearchFilter;

	UPROPERTY(EditAnywhere, Instanced, Category = "Commandlet")
	TObjectPtr<UAVCommandletAction> Action;
};
