// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetValidationModule.h"

#include "AssetValidationStatics.h"
#include "AssetValidationStyle.h"
#include "EditorValidatorSubsystem.h"
#include "ISourceControlModule.h"
#include "PropertyValidatorSubsystem.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "Editor/EngineVariableDescCustomization.h"
#include "Editor/PropertyValidationSettingsDetails.h"
#include "Misc/ScopedSlowTask.h"

DEFINE_LOG_CATEGORY(LogAssetValidation);

#define LOCTEXT_NAMESPACE "FAssetValidationModule"

class FAssetValidationModule : public IAssetValidationModule
{
public:

	//~Begin IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~End IModuleInterface
	
	virtual FPropertyValidationResult ValidateProperty(UObject* Object, FProperty* Property) const override;
	
private:

	static void CheckContent();
	static void CheckProjectSettings();
	static void ValidateChangelistPreSubmit(FSourceControlChangelistPtr Changelist, EDataValidationResult& OutResult, TArray<FText>& ValidationErrors, TArray<FText>& ValidationWarnings);
	static bool HasNoPlayWorld();
	void RegisterMenus();
};

void FAssetValidationModule::StartupModule()
{
	FAssetValidationStyle::Initialize();
	
	if (FSlateApplication::IsInitialized())
	{
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAssetValidationModule::RegisterMenus));
	}
	
	ISourceControlModule::Get().RegisterPreSubmitDataValidation(FSourceControlPreSubmitDataValidationDelegate::CreateStatic(&FAssetValidationModule::ValidateChangelistPreSubmit));

	FPropertyEditorModule& PropertyEditor = FModuleManager::Get().LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditor.RegisterCustomClassLayout("PropertyValidationSettings", FOnGetDetailCustomizationInstance::CreateStatic(&FPropertyValidationSettingsDetails::MakeInstance));
	PropertyEditor.RegisterCustomPropertyTypeLayout("EngineVariableDescription", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FEngineVariableDescCustomization::MakeInstance));
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
	
	ISourceControlModule::Get().UnregisterPreSubmitDataValidation();
}

FPropertyValidationResult FAssetValidationModule::ValidateProperty(UObject* Object, FProperty* Property) const
{
	return GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>()->ValidateObjectProperty(Object, Property);
}

void FAssetValidationModule::CheckContent()
{
	if (!ISourceControlModule::Get().IsEnabled())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("SourceControlDisabled", "Your source control is disabled. Enable and try again."));
		return;
	}

	TArray<FString> Warnings, Errors;
	AssetValidationStatics::ValidateSourceControl(true, EDataValidationUsecase::Manual, Warnings, Errors);
}

void FAssetValidationModule::CheckProjectSettings()
{
	if (!ISourceControlModule::Get().IsEnabled())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("SourceControlDisabled", "Your source control is disabled. Enable and try again."));
		return;
	}
	
	{
		FScopedSlowTask SlowTask(0.f, LOCTEXT("CheckProjectSetings", "Checking project settings..."));
		SlowTask.MakeDialog();
		
		TArray<FString> Warnings, Errors;
		AssetValidationStatics::ValidateProjectSettings(EDataValidationUsecase::Manual, Warnings, Errors);
	}

}

void FAssetValidationModule::ValidateChangelistPreSubmit(FSourceControlChangelistPtr Changelist, EDataValidationResult& OutResult, TArray<FText>& ValidationErrors, TArray<FText>& ValidationWarnings)
{
	TArray<FString> Warnings, Errors;
	AssetValidationStatics::ValidateSourceControl(true, EDataValidationUsecase::Manual, Warnings, Errors);

	OutResult = Errors.Num() > 0 ? EDataValidationResult::Invalid : EDataValidationResult::Valid;

	auto Trans = [](const FString& Str) { return FText::FromString(Str); };
	Algo::Transform(Errors, ValidationErrors, Trans);
	Algo::Transform(Warnings, ValidationWarnings, Trans);
}

bool FAssetValidationModule::HasNoPlayWorld()
{
	return GEditor->PlayWorld == nullptr;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAssetValidationModule, AssetValidation)