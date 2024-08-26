#include "LevelEditorCustomization.h"

#include "AssetValidationStyle.h"
#include "PropertyValidatorSubsystem.h"
#include "Commandlet/AVCommandletAction.h"
#include "Commandlet/AVCommandletSearchFilter.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

FAssetValidationToolkitManager::FAssetValidationToolkitManager()
{
	UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu("MainFrame.MainMenu.Window");
	FToolMenuSection& LogSection = WindowMenu->FindOrAddSection("Log");

	LogSection.AddMenuEntry("OpenAssetValidation",
		LOCTEXT("WindowItemName", "Asset Validation"),
		LOCTEXT("WindowItemTooltip", "Opens the Asset Validation Toolkit"),
		FAssetValidationStyle::GetValidateMenuIcon(),
		FUIAction{FExecuteAction::CreateRaw(this, &ThisClass::SummonTab)}
	);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TabName, FOnSpawnTab::CreateRaw(this, &ThisClass::CreateTabBody))
	.SetDisplayName(LOCTEXT("WindowItemName", "Asset Validation"))
	.SetIcon(FAssetValidationStyle::GetValidateMenuIcon())
	.SetAutoGenerateMenuEntry(false);
}

FAssetValidationToolkitManager::~FAssetValidationToolkitManager()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TabName);
}

void FAssetValidationToolkitManager::SummonTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(TabName);
}

TSharedRef<SDockTab> FAssetValidationToolkitManager::CreateTabBody(const FSpawnTabArgs& TabArgs) const
{
	return SNew(SDockTab)
	.TabRole(NomadTab)
	[
		SNew(SAssetValidationToolkit)
	];
}

void SAssetValidationToolkit::Construct(const FArguments& Args)
{
	FPropertyEditorModule& PropertyEditor = FModuleManager::Get().GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = true;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.NotifyHook = this;

	DetailsView = PropertyEditor.CreateDetailView(DetailsViewArgs);

	ToolkitView = NewObject<UAssetValidationToolkitView>();
	DetailsView->SetObject(ToolkitView.Get());

	ChildSlot
	[
		DetailsView.ToSharedRef()
	];
}

void SAssetValidationToolkit::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(ToolkitView);
}

void SAssetValidationToolkit::NotifyPreChange(FProperty* PropertyAboutToChange)
{
	// DetailsView->SetObject(nullptr);
}

void SAssetValidationToolkit::NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, FProperty* PropertyThatChanged)
{
	// DetailsView->SetObject(ToolkitView.Get(), true);
}

void UAssetValidationToolkitView::CheckContent()
{
}

void UAssetValidationToolkitView::CheckProjectSettings()
{
}

void UAssetValidationToolkitView::ExecuteCommandlet()
{
	CommandletStatus.Empty();
	CommandletStatus += SearchFilter ? TEXT("") : TEXT("Seach filter is required.\n");
	CommandletStatus += Action ? TEXT("") : TEXT("Action is required.\n");

	if (!CommandletStatus.IsEmpty())
	{
		CommandletStatus.RemoveFromEnd(TEXT("\n"));
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

	CommandletStatus.RemoveFromEnd(TEXT("\n"));
}

#undef LOCTEXT_NAMESPACE
