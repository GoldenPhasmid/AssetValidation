#include "ValidationEditorExtensionManager.h"

#include "BlueprintEditor.h"
#include "BlueprintEditorCustomization.h"
#include "BlueprintEditorModule.h"
#include "BlueprintEditorTabs.h"
#include "BlueprintComponentCustomization.h"
#include "BlueprintVariableCustomization.h"
#include "PropertyValidationSettings.h"
#include "StructureEditorCustomization.h"
#include "SubobjectData.h"
#include "SubobjectDataSubsystem.h"
#include "Framework/Docking/LayoutExtender.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "PropertyValidators/PropertyValidation.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"
#include "Engine/UserDefinedStruct.h"

const FName UValidationEditorExtensionManager::ValidationTabId{TEXT("ValidationTab")};

UValidationEditorExtensionManager::UValidationEditorExtensionManager()
{

}

void UValidationEditorExtensionManager::Tick(float DeltaTime)
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}
	const FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	for (TSharedRef<IBlueprintEditor> Editor: BlueprintEditorModule.GetBlueprintEditors())
	{
		TSharedRef<FBlueprintEditor> BlueprintEditor = StaticCastSharedRef<FBlueprintEditor>(Editor);
		if (BlueprintEditor->CanAccessComponentsMode())
		{
			TSharedPtr<IDetailsView> PropertyView = BlueprintEditor->GetInspector()->GetPropertyView();

			if (FOnGetDetailCustomizationInstance Delegate = PropertyView->GetGenericLayoutDetailsDelegate(); !Delegate.IsBoundToObject(this))
			{
				// do not capture blueprint editor by hard reference, as it is responsible for a lot of stuff that should be destroyed once editor closed
				FOnGetDetailCustomizationInstance ProxyDelegate = FOnGetDetailCustomizationInstance::CreateUObject(this, &ThisClass::HandleInspectorDefaultLayoutRequested, BlueprintEditor.ToWeakPtr(), Delegate);
				PropertyView->SetGenericLayoutDetailsDelegate(ProxyDelegate);
			}
		}
	}
}

TSharedRef<IDetailCustomization> UValidationEditorExtensionManager::HandleInspectorDefaultLayoutRequested(TWeakPtr<FBlueprintEditor> WeakEditor, FOnGetDetailCustomizationInstance ChildDelegate)
{
	struct FEmptyDetailCustomization: public IDetailCustomization
	{
		virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override {}
	};

	if (WeakEditor.IsValid())
	{
		// this chain is ugly but seems to always work
		for (TWeakObjectPtr<UObject> SelectedObject: WeakEditor.Pin()->GetInspector()->GetPropertyView()->GetSelectedObjects())
		{
			if (SelectedObject.IsValid() && SelectedObject->IsA<UActorComponent>())
			{
				return UE::AssetValidation::FBlueprintComponentCustomization::MakeInstance(WeakEditor.Pin(), ChildDelegate).ToSharedRef();
			}
		}
	}

	
	if (ChildDelegate.IsBound())
	{
		return ChildDelegate.Execute();
	}

	return MakeShared<FEmptyDetailCustomization>();
}

void UValidationEditorExtensionManager::Initialize()
{
	// Register bp variable customization
	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	VariableCustomizationHandle = BlueprintEditorModule.RegisterVariableCustomization(FProperty::StaticClass(),
		FOnGetVariableCustomizationInstance::CreateStatic(&UE::AssetValidation::FBlueprintVariableCustomization::MakeInstance));
	BlueprintTabSpawnerHandle = BlueprintEditorModule.OnRegisterTabsForEditor().AddUObject(this, &ThisClass::RegisterValidationTab);
	BlueprintLayoutExtensionHandle = BlueprintEditorModule.OnRegisterLayoutExtensions().AddUObject(this, &ThisClass::RegisterBlueprintEditorLayout);
	
	USubobjectDataSubsystem* SubobjectSubsystem = GEngine->GetEngineSubsystem<USubobjectDataSubsystem>();
	SubobjectAddedHandle = SubobjectSubsystem->OnNewSubobjectAdded().AddUObject(this, &ThisClass::HandleBlueprintComponentAdded);

	// react to blueprint changes by listening to Modify()
	ObjectModifiedHandle = FCoreUObjectDelegates::OnObjectModified.AddUObject(this, &ThisClass::PreBlueprintChange);

	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	AssetEditorSubsystem->OnEditorOpeningPreWidgets().AddUObject(this, &ThisClass::HandleAssetEditorPreConstruction);
	AssetEditorSubsystem->OnAssetEditorOpened().AddUObject(this, &ThisClass::HandleAssetEditorOpened);
	AssetEditorSubsystem->OnAssetEditorRequestClose().AddUObject(this, &ThisClass::HandleAssetEditorClosed);;
}

void UValidationEditorExtensionManager::Cleanup()
{
	FCoreUObjectDelegates::OnObjectModified.Remove(ObjectModifiedHandle);

	if (FModuleManager::Get().IsModuleLoaded("Kismet"))
	{
		// Unregister bp variable customization
		FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::GetModuleChecked<FBlueprintEditorModule>("Kismet");
		BlueprintEditorModule.UnregisterVariableCustomization(FProperty::StaticClass(), VariableCustomizationHandle);
		BlueprintEditorModule.OnRegisterTabsForEditor().Remove(BlueprintTabSpawnerHandle);
		BlueprintEditorModule.OnRegisterLayoutExtensions().Remove(BlueprintLayoutExtensionHandle);
	}
	
	if (!IsEngineExitRequested())
	{
		if (USubobjectDataSubsystem* SubobjectSubsystem = GEngine->GetEngineSubsystem<USubobjectDataSubsystem>())
		{
			SubobjectSubsystem->OnNewSubobjectAdded().Remove(SubobjectAddedHandle);
		}
	}
}

void UValidationEditorExtensionManager::PreBlueprintChange(UObject* ModifiedObject)
{
	if (UBlueprint* Blueprint = Cast<UBlueprint>(ModifiedObject))
	{
		if (!CachedBlueprints.Contains(Blueprint))
		{
			FBlueprintVariableData BlueprintData{Blueprint};
			BlueprintData.OnChangedHandle = Blueprint->OnChanged().AddUObject(this, &ThisClass::PostBlueprintChange);
			CachedBlueprints.Add(MoveTemp(BlueprintData));
		}
	}
}

void UValidationEditorExtensionManager::PostBlueprintChange(UBlueprint* Blueprint)
{
	const int32 Index = CachedBlueprints.IndexOfByKey(Blueprint);
	check(Index != INDEX_NONE);

	FBlueprintVariableData OldData = CachedBlueprints[Index];
	// remove cached data
	CachedBlueprints.RemoveAt(Index);
	Blueprint->OnChanged().Remove(OldData.OnChangedHandle);
	
	static bool bUpdatingVariables = false;
	TGuardValue UpdateVariablesGuard{bUpdatingVariables, true};

	TMap<FGuid, int32> NewVariables;
	for (int32 VarIndex = 0; VarIndex < Blueprint->NewVariables.Num(); ++VarIndex)
	{
		NewVariables.Add(Blueprint->NewVariables[VarIndex].VarGuid, VarIndex);
	}

	TMap<FGuid, int32> OldVariables;
	for (int32 VarIndex = 0; VarIndex < OldData.Variables.Num(); ++VarIndex)
	{
		OldVariables.Add(OldData.Variables[VarIndex].VarGuid, VarIndex);
	}

	for (const FBPVariableDescription& OldVariable: OldData.Variables)
	{
		if (!NewVariables.Contains(OldVariable.VarGuid))
		{
			HandleVariableRemoved(Blueprint, OldVariable.VarName);
		}
	}

	for (const FBPVariableDescription& NewVariable: Blueprint->NewVariables)
	{
		if (!OldVariables.Contains(NewVariable.VarGuid))
		{
			HandleVariableAdded(Blueprint, NewVariable.VarName);
			continue;
		}

		const int32 OldVarIndex = OldVariables.FindChecked(NewVariable.VarGuid);
		const FBPVariableDescription& OldVariable = OldData.Variables[OldVarIndex];
		if (OldVariable.VarName != NewVariable.VarName)
		{
			HandleVariableRenamed(Blueprint, OldVariable.VarName, NewVariable.VarName);
		}
		if (OldVariable.VarType != NewVariable.VarType)
		{
			HandleVariableTypeChanged(Blueprint, NewVariable.VarName, OldVariable.VarType, NewVariable.VarType);
		}
	}
}

void UValidationEditorExtensionManager::HandleVariableAdded(UBlueprint* Blueprint, const FName& VarName)
{
	if (!UPropertyValidationSettings::Get()->bAddMetaToNewBlueprintVariables)
	{
		return;
	}

	UpdateBlueprintVariableMetaData(Blueprint, VarName, UPropertyValidationSettings::Get()->bAddMetaToNewBlueprintVariables);
}

void UValidationEditorExtensionManager::HandleVariableTypeChanged(UBlueprint* Blueprint, const FName& VarName, FEdGraphPinType OldPinType, FEdGraphPinType NewPinType)
{
	UpdateBlueprintVariableMetaData(Blueprint, VarName, UPropertyValidationSettings::Get()->bAddMetaToNewBlueprintVariables);
}

void UValidationEditorExtensionManager::UpdateBlueprintVariableMetaData(UBlueprint* Blueprint, const FName& VarName, bool bAddIfPossible)
{
	const FProperty* VarProperty = FindFProperty<FProperty>(Blueprint->SkeletonGeneratedClass, VarName);
	check(VarProperty);

	for (FName MetaKey: UE::AssetValidation::GetValidationMetaKeys())
	{
		UE::AssetValidation::UpdateBlueprintVarMetaData(Blueprint, VarProperty, VarName, MetaKey, bAddIfPossible);
	}
}

void UValidationEditorExtensionManager::RegisterValidationTab(FWorkflowAllowedTabSet& TabFactory, FName ModeName, TSharedPtr<FBlueprintEditor> BlueprintEditor)
{
	TabFactory.RegisterFactory(MakeShared<FValidationTabSummoner>(BlueprintEditor));
}

void UValidationEditorExtensionManager::RegisterBlueprintEditorLayout(FLayoutExtender& Extender)
{
	Extender.ExtendLayout(FBlueprintEditorTabs::DetailsID, ELayoutExtensionPosition::Before, FTabManager::FTab(ValidationTabId, ETabState::OpenedTab));
}

void UValidationEditorExtensionManager::HandleAssetEditorPreConstruction(const TArray<UObject*>& Assets, IAssetEditorInstance* AssetEditor)
{
	if (Assets.Num() == 0)
	{
		return;
	}
	
	UObject* Asset = Assets[0];
	if (!Asset->IsA<UUserDefinedStruct>())
	{
		return;
	}

	FAssetEditorToolkit* AssetToolkit = static_cast<FAssetEditorToolkit*>(AssetEditor);
	if (TSharedPtr<FTabManager> TabManager = AssetToolkit->GetTabManager(); TabManager.IsValid())
	{
		TabManager->UnregisterTabSpawner(ValidationTabId);
		FTabSpawnerEntry& TabSpawner = TabManager->RegisterTabSpawner(ValidationTabId, FOnSpawnTab::CreateUObject(this, &ThisClass::SpawnValidationTab, Asset));
		TabSpawner.SetDisplayName(NSLOCTEXT("AssetValidation", "ValidationTabLabel", "Validation"));
		TabSpawner.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon"));

		if (TSharedPtr<FWorkspaceItem> WorkspaceRoot = TabManager->GetLocalWorkspaceMenuRoot())
		{
			if (WorkspaceRoot->GetChildItems().Num() > 0)
			{
				const TSharedRef<FWorkspaceItem>& FirstCategory = WorkspaceRoot->GetChildItems()[0];
				TabSpawner.SetGroup(FirstCategory);
			}
		}
	}
}

TSharedRef<SDockTab> UValidationEditorExtensionManager::SpawnValidationTab(const FSpawnTabArgs& Args, UObject* Asset)
{
	UUserDefinedStruct* EditedStruct = CastChecked<UUserDefinedStruct>(Asset);
	
	return SNew(SDockTab)
	.Label(NSLOCTEXT("AssetValidation", "ValidationTabLabel", "Validation"))
	.TabColorScale(FLinearColor{ForceInitToZero})
	[
		SNew(SStructureEditorValidationTab)
		.Struct(EditedStruct)
	];
}

void UValidationEditorExtensionManager::HandleAssetEditorOpened(UObject* Asset)
{
	if (!Asset->IsA<UUserDefinedStruct>())
	{
		return;
	}
	
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (IAssetEditorInstance* AssetEditor = AssetEditorSubsystem->FindEditorForAsset(Asset, false))
	{
		FAssetEditorToolkit* AssetToolkit = static_cast<FAssetEditorToolkit*>(AssetEditor);

		AssetToolkit->InvokeTab(ValidationTabId);
		AssetToolkit->InvokeTab(FName{TEXT("UserDefinedStruct_MemberVariablesEditor")});
	}
}

void UValidationEditorExtensionManager::HandleAssetEditorClosed(UObject* Asset, EAssetEditorCloseReason CloseReason)
{
}


void UValidationEditorExtensionManager::HandleBlueprintComponentAdded(const FSubobjectData& NewSubobjectData)
{
	if (!UPropertyValidationSettings::Get()->bAddMetaToNewBlueprintComponents)
	{
		// component automatic validation is disabled
		return;
	}

	if (NewSubobjectData.IsComponent() && !NewSubobjectData.IsInheritedComponent())
	{
		if (UBlueprint* Blueprint = NewSubobjectData.GetBlueprint())
		{
			const FName VariableName = NewSubobjectData.GetVariableName();

			const FString ComponentNotValid = FString::Printf(TEXT("Corrupted component property of name %s"), *VariableName.ToString());
			FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, VariableName, nullptr, UE::AssetValidation::Validate, {});
			FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, VariableName, nullptr, UE::AssetValidation::ValidateRecursive, {});
			FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, VariableName, nullptr, UE::AssetValidation::FailureMessage, ComponentNotValid);

			if (const UClass* SkeletonClass = Blueprint->SkeletonGeneratedClass)
			{
				const FProperty* ComponentProperty = FindFProperty<FProperty>(SkeletonClass, VariableName);
				check(ComponentProperty);
			}
		}
	}
}
