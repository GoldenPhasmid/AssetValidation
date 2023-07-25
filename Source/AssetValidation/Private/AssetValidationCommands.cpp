// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetValidationCommands.h"

#define LOCTEXT_NAMESPACE "FAssetValidationModule"

void FAssetValidationCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "AssetValidation", "Execute AssetValidation action", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
