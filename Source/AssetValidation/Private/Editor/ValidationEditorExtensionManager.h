#pragma once

#include "CoreMinimal.h"

#include "ValidationEditorExtensionManager.generated.h"

class FWorkflowAllowedTabSet;
class UBlueprint;
class FBlueprintEditor;
class FLayoutExtender;
struct FSubobjectData;

struct FBlueprintVariableData
{
	FBlueprintVariableData(UBlueprint* InBlueprint)
		: Blueprint(InBlueprint)
		, Variables(InBlueprint->NewVariables)
	{}

	FORCEINLINE bool operator==(UBlueprint* OtherBlueprint) const
	{
		return Blueprint.Get() == OtherBlueprint;
	}
	
	TWeakObjectPtr<UBlueprint> Blueprint;
	TArray<FBPVariableDescription> Variables;
	FDelegateHandle OnChangedHandle;
};

UCLASS(Within = PropertyValidatorSubsystem)
class ASSETVALIDATION_API UValidationEditorExtensionManager: public UObject, public FTickableEditorObject
{
	GENERATED_BODY()
public:

	UValidationEditorExtensionManager();

	//~Begin FTickableEditorObject interface
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UValidationEditorExtensionManager, STATGROUP_Tickables); }
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
	//~End FTickableEditorObject interface

	void Initialize();
	void Cleanup();

	/** Update variable meta data based on its property type and already placed metas */
	void UpdateBlueprintVariableMetaData(UBlueprint* Blueprint, const FName& VarName, bool bAddIfPossible);

private:
	
	static const FName ValidationTabId;
	TSharedRef<SDockTab> SpawnValidationTab(const FSpawnTabArgs& Args, UObject* Asset);

	/** Blueprint editor extension callbacks */
	void RegisterValidationTab(FWorkflowAllowedTabSet& TabFactory, FName ModeName, TSharedPtr<FBlueprintEditor> BlueprintEditor);
	void RegisterBlueprintEditorLayout(FLayoutExtender& Extender);
	
	/** Callbacks for editor open/close events */
	void HandleAssetEditorPreConstruction(const TArray<UObject*>& Assets, IAssetEditorInstance* AssetEditor);
	/** Asset editor is fully opened for specified asset */
	void HandleAssetEditorOpened(UObject* Asset);
	/** Asset editor is about to close for specified asset */
	void HandleAssetEditorClosed(UObject* Asset, EAssetEditorCloseReason CloseReason);
	
	/** Callbacks for blueprint variable events  */
	void HandleVariableAdded(UBlueprint* Blueprint, const FName& VarName);
	void HandleVariableRemoved(UBlueprint* Blueprint, const FName& VariableName) {};
	void HandleVariableRenamed(UBlueprint* Blueprint, const FName& OldName, const FName& NewName) {};
	void HandleVariableTypeChanged(UBlueprint* Blueprint, const FName& VarName, FEdGraphPinType OldPinType, FEdGraphPinType NewPinType);

	TSharedRef<IDetailCustomization> HandleInspectorDefaultLayoutRequested(TSharedRef<FBlueprintEditor> BlueprintEditor, FOnGetDetailCustomizationInstance ChildDelegate);

	/** Pre blueprint change callback */
	void PreBlueprintChange(UObject* ModifiedObject);
	/** Post blueprint change callback */
	void PostBlueprintChange(UBlueprint* Blueprint);

	void HandleBlueprintComponentAdded(const FSubobjectData& NewSubobjectData);

	/** Cached blueprint data between PreBlueprintChange and PostBlueprintChange */
	TArray<FBlueprintVariableData> CachedBlueprints;
	/** Blueprint variable customization handle */
	FDelegateHandle VariableCustomizationHandle;
	/** */
	FDelegateHandle BlueprintTabSpawnerHandle;
	/** */
	FDelegateHandle BlueprintLayoutExtensionHandle;
	/** Blueprint object has been modified handle */
	FDelegateHandle ObjectModifiedHandle;
	/** New subobject added handle */
	FDelegateHandle SubobjectAddedHandle;
};
