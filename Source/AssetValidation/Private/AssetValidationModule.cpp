// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetValidationModule.h"

#include "AssetValidationStatics.h"
#include "AssetValidationStyle.h"
#include "EditorValidatorSubsystem.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "SourceControlProxy.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "Editor/PropertyExternalValidationDataCustomization.h"
#include "Editor/PropertyValidationSettingsDetails.h"
#include "Misc/ScopedSlowTask.h"

#define LOCTEXT_NAMESPACE "FAssetValidationModule"

using UE::AssetValidation::ISourceControlProxy;

class FAssetValidationModule final : public IAssetValidationModule 
{
	using ThisClass = FAssetValidationModule;
public:

	//~Begin IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~End IModuleInterface
	
	virtual TSharedRef<ISourceControlProxy> GetSourceControlProxy() const override
	{
		return SourceControlProxy.ToSharedRef();
	}

private:

	void UpdateSourceControlProxy(ISourceControlProvider& OldProvider, ISourceControlProvider& NewProvider);

	TSharedPtr<ISourceControlProxy> SourceControlProxy;
	FDelegateHandle SCProviderChangedHandle;

#if 0
	struct FNativeClassMap
	{
		static const FName ModuleNameTag{"ModuleName"};
		static const FName ModuleRelativePathTag{"ModuleRelativePath"};

		FNativeClassMap()
		{
			for (TObjectIterator<UClass> It; It; ++It)
			{
				UClass* Class = *It;
				if (Class->HasAnyClassFlags(CLASS_Native))
				{
					FAssetData ClassData{Class};
					FString ModuleName = ClassData.GetTagValueRef<FString>(ModuleNameTag);
					FString ModulePath = ClassData.GetTagValueRef<FString>(ModuleRelativePathTag);

					const FString Key = ModuleName / ModulePath;
					Classes.FindOrAdd(Key).Add(Class);
				}
			}
		}
		
		TMap<FString, TArray<TWeakObjectPtr<UClass>>> Classes;
	};
#endif
	
	
	static void CheckContent();
	static void CheckProjectSettings();
	static bool HasNoPlayWorld();
	void RegisterMenus();
};

void FAssetValidationModule::UpdateSourceControlProxy(ISourceControlProvider& OldProvider, ISourceControlProvider& NewProvider)
{
	static const FName GitProviderName{"Git"};
	static const FName PerforceProviderName{"Perforce"};
	
	const FName ProviderName = NewProvider.GetName();
	if (ProviderName == GitProviderName)
	{
		SourceControlProxy = MakeShared<UE::AssetValidation::FGitSourceControlProxy>();
	}
	else if (ProviderName == PerforceProviderName)
	{
		SourceControlProxy = MakeShared<UE::AssetValidation::FPerforceSourceControlProxy>();
	}
	else
	{
		SourceControlProxy = MakeShared<UE::AssetValidation::FNullSourceControlProxy>();
	}
}

void FAssetValidationModule::StartupModule()
{
	FAssetValidationStyle::Initialize();
	
	if (FSlateApplication::IsInitialized())
	{
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &ThisClass::RegisterMenus));
	}
	
	ISourceControlModule& SourceControl = ISourceControlModule::Get();
	SCProviderChangedHandle = SourceControl.RegisterProviderChanged(FSourceControlProviderChanged::FDelegate::CreateRaw(this, &ThisClass::UpdateSourceControlProxy));

	ISourceControlProvider& SCCProvider = SourceControl.GetProvider();
	UpdateSourceControlProxy(SCCProvider, SCCProvider);
	
	FPropertyEditorModule& PropertyEditor = FModuleManager::Get().LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditor.RegisterCustomClassLayout("PropertyValidationSettings", FOnGetDetailCustomizationInstance::CreateStatic(&FPropertyValidationSettingsDetails::MakeInstance));
	PropertyEditor.RegisterCustomPropertyTypeLayout("PropertyExternalValidationData", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FPropertyExternalValidationDataCustomization::MakeInstance));
}

void FAssetValidationModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");

	const FUIAction CheckContentAction = FUIAction(
		FExecuteAction::CreateStatic(&FAssetValidationModule::CheckContent),
		FCanExecuteAction::CreateStatic(&FAssetValidationModule::HasNoPlayWorld),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateStatic(&FAssetValidationModule::HasNoPlayWorld)
	);
	
	FToolMenuSection& PlayGameSection = ToolbarMenu->AddSection("PlayGameExtensions", TAttribute<FText>(), FToolMenuInsert("Play", EToolMenuInsertType::After));
	
	FToolMenuEntry CheckContentEntry = FToolMenuEntry::InitToolBarButton(
		"CheckContent",
		CheckContentAction,
		LOCTEXT("CheckContent", "Check Content"),
		LOCTEXT("CheckContentDescription", "Runs Content Validation job on all checked out assets"),
		FSlateIcon(FAssetValidationStyle::GetStyleSetName(), "AssetValidation.CheckContent")
	);
	CheckContentEntry.StyleNameOverride = "CalloutToolbar";
	PlayGameSection.AddEntry(CheckContentEntry);

	UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	FToolMenuSection& ToolsSection = ToolsMenu->FindOrAddSection("DataValidation");
	
	ToolsSection.AddEntry(FToolMenuEntry::InitMenuEntry(
		"CheckContent",
		LOCTEXT("CheckContent", "Check Content"),
		LOCTEXT("CheckContentTooltip", "Check all source controlled assets."),
		FSlateIcon(FAssetValidationStyle::GetStyleSetName(), "AssetValidation.CheckContent"),
		CheckContentAction
	));
	ToolsSection.AddEntry(FToolMenuEntry::InitMenuEntry(
		"CheckProjectSettings",
		LOCTEXT("CheckProjectSettings", "Check Project Settings"),
		LOCTEXT("CheckProjectSettingsTooltip", "Check all config objects and developer settings"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon"),
		FUIAction(FExecuteAction::CreateStatic(&FAssetValidationModule::CheckProjectSettings))
	));

#if 0
	UToolMenu* BlueprintMenu = UToolMenus::Get()->ExtendMenu("MainFrame.MainMenu.Tools");
	BlueprintMenu->AddMenuEntry("DataValidation", FToolMenuEntry::InitMenuEntry(
		"BlueprintMenuTest",
		LOCTEXT("BlueprintMenuTest", "Menu Test"),
		FText::GetEmpty(),
		FSlateIcon{},
		FToolUIActionChoice{}
	));
#endif
}


void FAssetValidationModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FAssetValidationStyle::Shutdown();
	
	ISourceControlModule& SourceControl = ISourceControlModule::Get();
	SourceControl.UnregisterProviderChanged(SCProviderChangedHandle);
}

void FAssetValidationModule::CheckContent()
{
	if (!ISourceControlModule::Get().IsEnabled())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("SourceControlDisabled", "Your source control is disabled. Enable and try again."));
		return;
	}

	FValidateAssetsSettings Settings;
	Settings.ValidationUsecase = EDataValidationUsecase::Manual;
	Settings.bSkipExcludedDirectories = true;
	Settings.bShowIfNoFailures = true;
	
	FValidateAssetsResults Results;
	UE::AssetValidation::ValidateCheckedOutAssets(true, Settings, Results);
}

void FAssetValidationModule::CheckProjectSettings()
{
	FScopedSlowTask SlowTask(0.f, LOCTEXT("CheckProjectSetings", "Checking project settings..."));
	SlowTask.MakeDialog();
		
	FValidateAssetsSettings Settings;
	Settings.ValidationUsecase = EDataValidationUsecase::Manual;
	Settings.bSkipExcludedDirectories = true;
	Settings.bShowIfNoFailures = true;
	
	FValidateAssetsResults Results;
	UE::AssetValidation::ValidateProjectSettings(Settings, Results);
}

bool FAssetValidationModule::HasNoPlayWorld()
{
	return GEditor->PlayWorld == nullptr;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAssetValidationModule, AssetValidation)