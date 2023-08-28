// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetValidation.h"
#include "AssetValidationStyle.h"
#include "AssetValidationCommands.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"

static const FName AssetValidationTabName("AssetValidation");

DEFINE_LOG_CATEGORY(LogAssetValidation);

#define LOCTEXT_NAMESPACE "FAssetValidationModule"

class FAssetValidationModule : public IAssetValidationModule
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command. */
	void PluginButtonClicked();

	virtual void ValidateProperty(FProperty* Property, FPropertyValidationResult& OutValidationResult) const override;
	
private:

	void RegisterMenus();


private:
	TSharedPtr<class FUICommandList> PluginCommands;
};

void FAssetValidationModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FAssetValidationStyle::Initialize();
	FAssetValidationStyle::ReloadTextures();

	FAssetValidationCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FAssetValidationCommands::Get().PluginAction,
		FExecuteAction::CreateRaw(this, &FAssetValidationModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAssetValidationModule::RegisterMenus));
}

void FAssetValidationModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FAssetValidationStyle::Shutdown();

	FAssetValidationCommands::Unregister();
}

void FAssetValidationModule::ValidateProperty(FProperty* Property, FPropertyValidationResult& OutValidationResult) const
{
	
}

void FAssetValidationModule::PluginButtonClicked()
{
	// Put your "OnButtonClicked" stuff here
	FText DialogText = FText::Format(
							LOCTEXT("PluginButtonDialogText", "Add code to {0} in {1} to override this button's actions"),
							FText::FromString(TEXT("FAssetValidationModule::PluginButtonClicked()")),
							FText::FromString(TEXT("AssetValidation.cpp"))
					   );
	FMessageDialog::Open(EAppMsgType::Ok, DialogText);
}

void FAssetValidationModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
			Section.AddMenuEntryWithCommandList(FAssetValidationCommands::Get().PluginAction, PluginCommands);
		}
	}

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
			{
				FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(FAssetValidationCommands::Get().PluginAction));
				Entry.SetCommandList(PluginCommands);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAssetValidationModule, AssetValidation)