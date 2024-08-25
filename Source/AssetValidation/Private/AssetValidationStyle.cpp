// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetValidationStyle.h"
#include "AssetValidationModule.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/SlateStyleRegistry.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FAssetValidationStyle::StyleInstance = nullptr;

void FAssetValidationStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}

	FAssetValidationStyle::ReloadTextures();
}

void FAssetValidationStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FAssetValidationStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("AssetValidation"));
	return StyleSetName;
}

FSlateIcon FAssetValidationStyle::GetCheckContentIcon()
{
	static FSlateIcon Icon{FAssetValidationStyle::GetStyleSetName(), "AssetValidation.CheckContent"};
	return Icon;
}

FSlateIcon FAssetValidationStyle::GetValidateMenuIcon()
{
	static FSlateIcon Icon{FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon"};
	return Icon;
}

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef< FSlateStyleSet > FAssetValidationStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet(GetStyleSetName()));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("AssetValidation")->GetBaseDir() / TEXT("Resources"));

	Style->Set("AssetValidation.CheckContent", new IMAGE_BRUSH_SVG(TEXT("CheckContentButtonIcon"), Icon20x20));
	return Style;
}

void FAssetValidationStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FAssetValidationStyle::Get()
{
	return *StyleInstance;
}
