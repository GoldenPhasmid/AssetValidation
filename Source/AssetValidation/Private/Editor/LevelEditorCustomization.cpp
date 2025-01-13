#include "LevelEditorCustomization.h"

#include "AssetDependencyViewerWidget.h"
#include "AssetValidationSettings.h"
#include "AssetValidationStatics.h"
#include "AssetValidationStyle.h"
#include "EditorValidatorHelpers.h"
#include "PropertyValidatorSubsystem.h"
#include "Commandlet/AVCommandletAction.h"
#include "Commandlet/AVCommandletSearchFilter.h"
#include "ISourceControlModule.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

namespace UE::AssetValidation
{
	const FName AVToolkit_Tab_CommandletPreview{TEXT("CommandletPreview")};
	const FName AVToolkit_Tab_DependencyViewer{TEXT("DependencyViewer")};
}

bool HasNoPlayWorld()
{
	return GEditor->PlayWorld == nullptr;
}

void ValidateCheckedOutAssets(EDataValidationUsecase Usecase)
{
	if (!ISourceControlModule::Get().IsEnabled())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("SourceControlDisabled", "Your source control is disabled. Enable and try again."));
		return;
	}

	FScopedSlowTask SlowTask(0.f, FText::FromString("Verifying checkout out assets..."));
	SlowTask.MakeDialog();
    
	FValidateAssetsSettings Settings;
	Settings.ValidationUsecase = Usecase;
	Settings.bSkipExcludedDirectories = true;
	Settings.bShowIfNoFailures = true;
    
	FMessageLog DataValidationLog(UE::DataValidation::MessageLogName);
	DataValidationLog.NewPage(LOCTEXT("CheckContent", "Check Content"));

	FValidateAssetsResults Results{};
	UE::AssetValidation::ValidateCheckedOutAssets(true, Settings, Results);
}
	
void ValidateProjectSettings(EDataValidationUsecase Usecase)
{
	FScopedSlowTask SlowTask(0.f, FText::FromString("Verifying project settings..."));
	SlowTask.MakeDialog();
		
	FValidateAssetsSettings Settings;
	Settings.ValidationUsecase = Usecase;
	Settings.bSkipExcludedDirectories = true;
	Settings.bShowIfNoFailures = true;
	
	FMessageLog DataValidationLog(UE::DataValidation::MessageLogName);
	DataValidationLog.NewPage(FText::FromString("Check Project Settings"));
	
	FValidateAssetsResults Results;
	UE::AssetValidation::ValidateProjectSettings(Settings, Results);
}

FAssetValidationMenuExtensionManager::FAssetValidationMenuExtensionManager()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(UE::AssetValidation::AVToolkit_Tab_CommandletPreview,
		FOnSpawnTab::CreateRaw(this, &ThisClass::CreateCommandletPreview))
	.SetDisplayName(LOCTEXT("CommandletPreview_WindowItemName", "AV Commandlet Preview"))
	.SetIcon(FAssetValidationStyle::GetValidateMenuIcon())
	.SetAutoGenerateMenuEntry(false);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(UE::AssetValidation::AVToolkit_Tab_DependencyViewer,
		FOnSpawnTab::CreateRaw(this, &ThisClass::CreateDependencyViewer))
	.SetDisplayName(LOCTEXT("DependencyViewer_WindowItemName", "AV Dependency Viewer"))
	.SetIcon(FAssetValidationStyle::GetValidateMenuIcon())
	.SetAutoGenerateMenuEntry(false);

	// TOOLS header
	UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	FToolMenuSection& ToolsSection = ToolsMenu->FindOrAddSection("Tools");

	const FUIAction CheckContentAction = FUIAction(
		FExecuteAction::CreateStatic(&ValidateCheckedOutAssets, EDataValidationUsecase::Script),
		FCanExecuteAction::CreateStatic(&HasNoPlayWorld),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateStatic(&HasNoPlayWorld)
	);
	
	ToolsSection.AddSubMenu("AssetValidation", FText::FromString(TEXT("AssetValidation")), FText::GetEmpty(),
		FNewToolMenuDelegate::CreateLambda([this, CheckContentAction](UToolMenu* InSubMenu)
	{
		FToolMenuOwnerScoped OwnerScoped(this);
			
		// Commandlet Preview
		InSubMenu->AddMenuEntry("CommandletPreview", FToolMenuEntry::InitMenuEntry(
			FName{"CommandletPreview"},
			LOCTEXT("CommandletPreview_WindowItemName", "AV Commandlet Preview"),
			LOCTEXT("CommandletPreview_WindowItemTooltip", "Opens the Asset Validation Toolkit"),
			FAssetValidationStyle::GetValidateMenuIcon(),
			FUIAction{FExecuteAction::CreateRaw(this, &ThisClass::SummonTab, UE::AssetValidation::AVToolkit_Tab_CommandletPreview)})
		);

		// Dependency Viewer
		InSubMenu->AddMenuEntry("DependencyViewer", FToolMenuEntry::InitMenuEntry(
			FName{"DependencyViewer"},
			LOCTEXT("DependencyViewer_WindowItemName", "AV Dependency Viewer"),
			LOCTEXT("DependencyViewer_WindowItemTooltip", "Opens the Asset Validation Toolkit"),
			FAssetValidationStyle::GetValidateMenuIcon(),
			FUIAction{FExecuteAction::CreateRaw(this, &ThisClass::SummonTab, UE::AssetValidation::AVToolkit_Tab_DependencyViewer)})
		);

		// CheckContent inside Tools header entry
		InSubMenu->AddMenuEntry("CheckContent", FToolMenuEntry::InitMenuEntry(
			"CheckContent",
			LOCTEXT("CheckContent", "Check Content"),
			LOCTEXT("CheckContentTooltip", "Check all source controlled assets."),
			FAssetValidationStyle::GetCheckContentIcon(),
			CheckContentAction
		));
			
		// CheckProjectSettings inside Tools header entry
		InSubMenu->AddMenuEntry("CheckProjectSettings", FToolMenuEntry::InitMenuEntry(
			"CheckProjectSettings",
			LOCTEXT("CheckProjectSettings", "Check Project Settings"),
			LOCTEXT("CheckProjectSettingsTooltip", "Check all config objects and developer settings"),
			FAssetValidationStyle::GetValidateMenuIcon(),
			FUIAction(FExecuteAction::CreateStatic(&ValidateProjectSettings, EDataValidationUsecase::Script))
		));
	}), false, FAssetValidationStyle::GetValidateMenuIcon());
	
	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
	FToolMenuSection& PlayGameSection = ToolbarMenu->AddSection("PlayGameExtensions", TAttribute<FText>(), FToolMenuInsert("Play", EToolMenuInsertType::After));

	// CheckContent on the LevelEditor toolbar
	FToolMenuEntry CheckContentEntry = FToolMenuEntry::InitToolBarButton(
		"CheckContent",
		CheckContentAction,
		LOCTEXT("CheckContent", "Check Content"),
		LOCTEXT("CheckContentDescription", "Runs Content Validation job on all checked out assets"),
		FAssetValidationStyle::GetCheckContentIcon()
	);
	CheckContentEntry.StyleNameOverride = "CalloutToolbar";
	PlayGameSection.AddEntry(CheckContentEntry);
}

FAssetValidationMenuExtensionManager::~FAssetValidationMenuExtensionManager()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(UE::AssetValidation::AVToolkit_Tab_CommandletPreview);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(UE::AssetValidation::AVToolkit_Tab_DependencyViewer);
	
	UToolMenus::UnregisterOwner(this);
}

void FAssetValidationMenuExtensionManager::SummonTab(FName TabName)
{
	FGlobalTabmanager::Get()->TryInvokeTab(TabName);
}

TSharedRef<SDockTab> FAssetValidationMenuExtensionManager::CreateCommandletPreview(const FSpawnTabArgs& TabArgs) const
{
	return SNew(SDockTab)
	.TabRole(NomadTab)
	[
		SNew(SAVCommandletPreviewWidget)
	];
}

TSharedRef<SDockTab> FAssetValidationMenuExtensionManager::CreateDependencyViewer(const FSpawnTabArgs& TabArgs) const
{
	return SNew(SDockTab)
	.TabRole(NomadTab)
	[
		SNew(SAssetDependencyViewerWidget)
	];
}

void SAVCommandletPreviewWidget::Construct(const FArguments& Args)
{
	FPropertyEditorModule& PropertyEditor = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.NotifyHook = this;

	DetailsView = PropertyEditor.CreateDetailView(DetailsViewArgs);

	ToolkitView = NewObject<UAVCommandletPreview>();
	DetailsView->SetObject(ToolkitView.Get());

	ChildSlot
	[
		DetailsView.ToSharedRef()
	];
}

void SAVCommandletPreviewWidget::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(ToolkitView);
}

void SAVCommandletPreviewWidget::NotifyPreChange(FProperty* PropertyAboutToChange)
{
	// noop
}

void SAVCommandletPreviewWidget::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	// noop
}

void UAVCommandletPreview::PostInitProperties()
{
	UObject::PostInitProperties();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		SearchFilter = NewObject<UAVCommandletSearchFilter>(this, UAssetValidationSettings::Get()->CommandletDefaultFilter);
		Action = NewObject<UAVCommandletAction>(this, UAssetValidationSettings::Get()->CommandletDefaultAction);
	}
}

void UAVCommandletPreview::CheckContent()
{
	ValidateCheckedOutAssets(EDataValidationUsecase::Commandlet);
}

void UAVCommandletPreview::CheckProjectSettings()
{
	ValidateProjectSettings(EDataValidationUsecase::Commandlet);
}

void UAVCommandletPreview::ExecuteCommandlet()
{
	ON_SCOPE_EXIT
	{
		CommandletStatus.RemoveFromEnd(TEXT("\n"));
	};

	CommandletStatus.Empty();
	CommandletStatus += SearchFilter ? TEXT("") : TEXT("Commandlet search filter is required.\n");
	CommandletStatus += Action ? TEXT("") : TEXT("Commandlet action is required.\n");

	if (!CommandletStatus.IsEmpty())
	{
		return;
	}

	bool bPassedValidation = true;
	if (FPropertyValidationResult Result = UPropertyValidatorSubsystem::Get()->ValidateObject(SearchFilter); Result.ValidationResult == EDataValidationResult::Invalid)
	{
		CommandletStatus += FString::Printf(TEXT("Validation for search filter %s failed. See log for details.\n"), *GetNameSafe(SearchFilter));
		bPassedValidation = false;
	}
	if (FPropertyValidationResult Result = UPropertyValidatorSubsystem::Get()->ValidateObject(Action); Result.ValidationResult == EDataValidationResult::Invalid)
	{
		CommandletStatus += FString::Printf(TEXT("Validation for action %s failed. See log for details.\n"), *GetNameSafe(Action));
		bPassedValidation = false;
	}

	if (bPassedValidation)
	{
		TArray<FAssetData> Assets;
		SearchFilter->GetAssets(Assets);

		const bool bResult = Action->Run(Assets);
		CommandletStatus += FString::Printf(TEXT("Status: %s.\n"), bResult ? TEXT("Succeeded") : TEXT("Failed"));
	}
}

#undef LOCTEXT_NAMESPACE
