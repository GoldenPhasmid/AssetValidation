// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetValidation.h"
#include "AssetValidationStyle.h"
#include "DataValidationChangelist.h"
#include "EditorValidatorSubsystem.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "PropertyValidatorSubsystem.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "Misc/ScopedSlowTask.h"
#include "PropertyValidators/PropertyValidation.h"

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
	static bool HasNoPlayWorld();
	void RegisterMenus();
};

void FAssetValidationModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FAssetValidationStyle::Initialize();
	FAssetValidationStyle::ReloadTextures();
	
	if (FSlateApplication::IsInitialized())
	{
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAssetValidationModule::RegisterMenus));
	}
}

void FAssetValidationModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
	FToolMenuSection& Section = Menu->AddSection("PlayGameExtensions", TAttribute<FText>(), FToolMenuInsert("Play", EToolMenuInsertType::After));

	FToolMenuEntry CheckContentEntry = FToolMenuEntry::InitToolBarButton(
		"CheckContent",
		FUIAction(
			FExecuteAction::CreateStatic(&FAssetValidationModule::CheckContent),
			FCanExecuteAction::CreateStatic(&FAssetValidationModule::HasNoPlayWorld),
			FIsActionChecked(),
			FIsActionButtonVisible::CreateStatic(&FAssetValidationModule::HasNoPlayWorld)
		),
		LOCTEXT("CheckContent", "Check Content"),
		LOCTEXT("CheckContentDescription", "Runs Content Validation job on all checked out assets"),
		FSlateIcon(FAssetValidationStyle::GetStyleSetName(), "AssetValidation.CheckContent")
	);
	CheckContentEntry.StyleNameOverride = "CalloutToolbar";
	Section.AddEntry(CheckContentEntry);
}


void FAssetValidationModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FAssetValidationStyle::Shutdown();
}

FPropertyValidationResult FAssetValidationModule::ValidateProperty(UObject* Object, FProperty* Property) const
{
	return GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>()->IsPropertyValid(Object, Property);
}

void FAssetValidationModule::CheckContent()
{
	if (!ISourceControlModule::Get().IsEnabled())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("SourceControlDisabled", "Your source control is disabled. Enable and try again."));
		return;
	}
	
	UEditorValidatorSubsystem* ContentValidation = GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>();

	ISourceControlProvider& SCCProvider = ISourceControlModule::Get().GetProvider();
	TArray<FSourceControlChangelistRef> Changelists = SCCProvider.GetChangelists(EStateCacheUsage::ForceUpdate);

	FScopedSlowTask SlowTask(Changelists.Num(), LOCTEXT("ValidateChangelistTask", "Validating changelists..."));
	SlowTask.MakeDialogDelayed(0.1f);
	
	for (const FSourceControlChangelistRef& ChangelistRef: Changelists)
	{
		UDataValidationChangelist* Changelist = NewObject<UDataValidationChangelist>(GetTransientPackage());
		Changelist->AddToRoot();
		Changelist->Initialize(ChangelistRef.ToSharedPtr());

		SlowTask.EnterProgressFrame(1.f, FText::GetEmpty());

		TArray<FText> Warnings, Errors;
		ContentValidation->IsObjectValid(Changelist, Errors, Warnings, EDataValidationUsecase::Manual);

		Changelist->RemoveFromRoot();
		Changelist->MarkAsGarbage();
	}
}

bool FAssetValidationModule::HasNoPlayWorld()
{
	return GEditor->PlayWorld == nullptr;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAssetValidationModule, AssetValidation)