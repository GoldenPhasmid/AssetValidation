﻿// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetValidationModule.h"

#include "AssetValidationSettings.h"
#include "AssetValidationStatics.h"
#include "AssetValidationStyle.h"
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
#include "Editor/PropertyMetaDataExtensionCustomization.h"
#include "Editor/LevelEditorCustomization.h"
#include "Editor/PropertyValidationSettingsCustomization.h"
#include "Editor/UnrealEdEngine.h"
#include "Misc/ScopedSlowTask.h"
#include "WorldPartition/WorldPartitionActorDescUtils.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

class IMessageToken;
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
	
	void OnAssetDataTokenActivated(const TSharedRef<IMessageToken>& InToken);
	FText OnGetAssetDataTokenDisplayName(const FAssetData& AssetData, const bool bFullPath);

	void UpdateSourceControlProxy(ISourceControlProvider& OldProvider, ISourceControlProvider& NewProvider);

	TSharedPtr<FAssetValidationMenuExtensionManager> ToolkitManager;
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
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([this]
		{
			ToolkitManager = MakeShared<FAssetValidationMenuExtensionManager>();
		}));
	}

	FPropertyEditorModule& PropertyEditor = FModuleManager::Get().LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditor.RegisterCustomClassLayout(StaticClass<UAssetValidationSettings>()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&UE::AssetValidation::FAssetValidationSettingsCustomization::MakeInstance));
	PropertyEditor.RegisterCustomClassLayout(StaticClass<UPropertyValidationSettings>()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&UE::AssetValidation::FPropertyValidationSettingsCustomization::MakeInstance));
	PropertyEditor.RegisterCustomPropertyTypeLayout(StaticStruct<FPropertyMetaDataExtension>()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&UE::AssetValidation::FPropertyMetaDataExtensionCustomization::MakeInstance));
	
	FEditorDelegates::OnEditorInitialized.AddLambda([this](double Duration)
	{
		// override default FAssetDataToken callbacks to further customize DisplayName and activation action
		FAssetDataToken::DefaultOnMessageTokenActivated().BindRaw(this, &ThisClass::OnAssetDataTokenActivated);
		FAssetDataToken::DefaultOnGetAssetDisplayName().BindRaw(this, &ThisClass::OnGetAssetDataTokenDisplayName);

		ISourceControlModule& SourceControl = ISourceControlModule::Get();
		SCProviderChangedHandle = SourceControl.RegisterProviderChanged(FSourceControlProviderChanged::FDelegate::CreateRaw(this, &ThisClass::UpdateSourceControlProxy));
		
		ISourceControlProvider& SCCProvider = SourceControl.GetProvider();
		UpdateSourceControlProxy(SCCProvider, SCCProvider);
	});
}

void FAssetValidationModule::ShutdownModule()
{
	ToolkitManager.Reset();
	UToolMenus::UnRegisterStartupCallback(this);
	
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FEditorDelegates::OnEditorInitialized.RemoveAll(this);
	
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

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAssetValidationModule, AssetValidation)