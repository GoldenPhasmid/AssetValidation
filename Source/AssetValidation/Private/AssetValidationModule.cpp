// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetValidationModule.h"

#include "AssetValidationSettings.h"
#include "AssetValidationStatics.h"
#include "AssetValidationStyle.h"
#include "EditorValidatorHelpers.h"
#include "EditorValidatorSubsystem.h"
#include "ISettingsModule.h"
#include "ISourceControlModule.h"
#include "ISourceControlProvider.h"
#include "PropertyExtensionTypes.h"
#include "SourceControlProxy.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "UnrealEdGlobals.h"
#include "AssetRegistry/AssetDataToken.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor/AssetValidationSettingsCustomization.h"
#include "Editor/EnginePropertyExtensionCustomization.h"
#include "Editor/PropertyValidationSettingsCustomization.h"
#include "Editor/UnrealEdEngine.h"
#include "Misc/ScopedSlowTask.h"
#include "WorldPartition/WorldPartitionActorDescUtils.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

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

	void OnAssetDataTokenActivated(const TSharedRef<class IMessageToken>& InToken);
	FText OnGetAssetDataTokenDisplayName(const FAssetData& AssetData, const bool bFullPath);

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
	PropertyEditor.RegisterCustomClassLayout(StaticClass<UAssetValidationSettings>()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&UE::AssetValidation::FAssetValidationSettingsCustomization::MakeInstance));
	PropertyEditor.RegisterCustomClassLayout(StaticClass<UPropertyValidationSettings>()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&UE::AssetValidation::FPropertyValidationSettingsCustomization::MakeInstance));
	PropertyEditor.RegisterCustomPropertyTypeLayout(StaticStruct<FEnginePropertyExtension>()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&UE::AssetValidation::FEnginePropertyExtensionCustomization::MakeInstance));

	FEditorDelegates::OnEditorInitialized.AddLambda([this](double Duration)
	{
		// override default FAssetDataToken callbacks to further customize DisplayName and activation action
		FAssetDataToken::DefaultOnMessageTokenActivated().BindRaw(this, &ThisClass::OnAssetDataTokenActivated);
		FAssetDataToken::DefaultOnGetAssetDisplayName().BindRaw(this, &ThisClass::OnGetAssetDataTokenDisplayName);
	});
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
	FEditorDelegates::OnEditorInitialized.RemoveAll(this);

	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);

	FAssetValidationStyle::Shutdown();
	
	ISourceControlModule& SourceControl = ISourceControlModule::Get();
	SourceControl.UnregisterProviderChanged(SCProviderChangedHandle);
}

void FAssetValidationModule::OnAssetDataTokenActivated(const TSharedRef<IMessageToken>& InToken)
{
	if (InToken->GetType() != EMessageToken::AssetData)
	{
		return;
	}
	
	const TSharedRef<FAssetDataToken> Token = StaticCastSharedRef<FAssetDataToken>(InToken);
	const FAssetData& AssetData = Token->GetAssetData();
	UObject* Asset = AssetData.FastGetAsset(false);

	// If this is a project settings default object, jump to respective project settings section
	if (Asset && Asset->HasAnyFlags(RF_ClassDefaultObject))
	{
		static const FName ContainerName = TEXT("Project");
		
		FName CategoryName	= Asset->GetClass()->ClassConfigName;
		FName SectionName	= Asset->GetClass()->GetFName();
		if (UDeveloperSettings* DeveloperSettings = Cast<UDeveloperSettings>(Asset))
		{
			CategoryName	= DeveloperSettings->GetCategoryName();
			SectionName		= DeveloperSettings->GetSectionName();
		}
		
		ISettingsModule& Settings = FModuleManager::LoadModuleChecked<ISettingsModule>("Settings");
		Settings.ShowViewer(ContainerName, CategoryName, SectionName);
	}
	// If this is a standalone asset, jump to it in the content browser
	else if (AssetData.GetOptionalOuterPathName().IsNone())
	{
		GEditor->SyncBrowserToObjects({ AssetData });
	}
	// If this is a loaded actor, resolve the pointer and select it
	else if (AActor* Actor = Cast<AActor>(Asset))
	{
		// copy from FUnrealEdMisc::SelectActorFromMessageToken
		// Select the actor
		GEditor->SelectNone(false, true);
		GEditor->SelectActor(Actor, /*InSelected=*/true, /*bNotify=*/false, /*bSelectEvenIfHidden=*/true);
		GEditor->NoteSelectionChange();
		GEditor->MoveViewportCamerasToActor(*Actor, false);

		// Update the property windows and create one if necessary
		GUnrealEd->ShowActorProperties();
		GUnrealEd->UpdateFloatingPropertyWindows();
	}
	else
	{
		FName PackageName = FName(FPackageName::ObjectPathToPackageName(WriteToString<FName::StringBufferSize>(AssetData.GetOptionalOuterPathName()).ToView()));
		UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
		// If this is an unloaded actor in the currently loaded level, select it as an unloaded actor
		if (EditorWorld && EditorWorld->GetPackage()->GetFName() == PackageName)
		{
			TUniquePtr<FWorldPartitionActorDesc> Desc = FWorldPartitionActorDescUtils::GetActorDescriptorFromAssetData(AssetData);
			if (Desc.IsValid())
			{
				GEditor->BroadcastSelectUnloadedActors({ Desc->GetGuid() });
			}
		}
		// If this is an actor in an unloaded level, jump to the level in the content browser
		else 
		{
			FAssetRegistryModule& AssetRegistryModule = FModuleManager::GetModuleChecked<FAssetRegistryModule>("AssetRegistry");
			IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
			TArray<FAssetData> OutAssets;
			AssetRegistry.GetAssetsByPackageName(PackageName, OutAssets);
			GEditor->SyncBrowserToObjects(OutAssets);
		}
	}
}

FText FAssetValidationModule::OnGetAssetDataTokenDisplayName(const FAssetData& AssetData, const bool bFullPath)
{
	static FName NAME_ActorLabel("ActorLabel");

	UObject* Asset = AssetData.FastGetAsset(false);
	
	FString DisplayName;
	TStringBuilder<FName::StringBufferSize> Buffer;
	if (AssetData.GetTagValue(NAME_ActorLabel, DisplayName) || 
		AssetData.GetTagValue(FPrimaryAssetId::PrimaryAssetDisplayNameTag, DisplayName))
	{
		// handles WP actors because of ActorLabel asset tag and primary assets
		Buffer << DisplayName;
		if (!UAssetValidationSettings::Get()->bUseShortActorNames)
		{
			Buffer << TEXT(" (");
			AssetData.AppendObjectPath(Buffer);
			Buffer << TEXT(")");
		}
		return FText::FromStringView(Buffer.ToView());
	}
	else if (AActor* Actor = Cast<AActor>(Asset))
	{
		// handles usual loaded actors
		Buffer << Actor->GetActorNameOrLabel() << TEXT(".") << AssetData.AssetName;
		return FText::FromStringView(Buffer.ToView());
	}
	else if (!AssetData.GetOptionalOuterPathName().IsNone())
	{
		// handles usual unloaded actors
		AssetData.AppendObjectPath(Buffer);
		return FText::FromStringView(Buffer.ToView());
	}
	else if (Asset && Asset->HasAnyFlags(RF_ClassDefaultObject))
	{
		// handles default objects like project settings
		FString AssetName = AssetData.AssetName.ToString();
		AssetName.RemoveFromStart(TEXT("Default__"));

		Buffer << AssetName;
		return FText::FromStringView(Buffer.ToView());
	}
	else if (bFullPath)
	{
		AssetData.AppendObjectPath(Buffer);
		return FText::FromStringView(Buffer.ToView());
	}
	return FText::FromName(AssetData.PackageName);
}

void FAssetValidationModule::CheckContent()
{
	if (!ISourceControlModule::Get().IsEnabled())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("SourceControlDisabled", "Your source control is disabled. Enable and try again."));
		return;
	}

	FValidateAssetsSettings Settings;
	Settings.ValidationUsecase = EDataValidationUsecase::Script;
	Settings.bSkipExcludedDirectories = true;
	Settings.bShowIfNoFailures = true;

	FMessageLog DataValidationLog(UE::DataValidation::MessageLogName);
	DataValidationLog.NewPage(LOCTEXT("CheckContent", "Check Content"));
	
	FValidateAssetsResults Results;
	UE::AssetValidation::ValidateCheckedOutAssets(true, Settings, Results);
}

void FAssetValidationModule::CheckProjectSettings()
{
	FScopedSlowTask SlowTask(0.f, LOCTEXT("CheckProjectSetings", "Checking project settings..."));
	SlowTask.MakeDialog();
		
	FValidateAssetsSettings Settings;
	Settings.ValidationUsecase = EDataValidationUsecase::Script;
	Settings.bSkipExcludedDirectories = true;
	Settings.bShowIfNoFailures = true;
	
	FMessageLog DataValidationLog(UE::DataValidation::MessageLogName);
	DataValidationLog.NewPage(LOCTEXT("CheckProjectSettings", "Check Project Settings"));
	
	FValidateAssetsResults Results;
	UE::AssetValidation::ValidateProjectSettings(Settings, Results);
}

bool FAssetValidationModule::HasNoPlayWorld()
{
	return GEditor->PlayWorld == nullptr;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAssetValidationModule, AssetValidation)