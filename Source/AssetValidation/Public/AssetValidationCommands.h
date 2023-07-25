// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "AssetValidationStyle.h"

class FAssetValidationCommands : public TCommands<FAssetValidationCommands>
{
public:

	FAssetValidationCommands()
		: TCommands<FAssetValidationCommands>(TEXT("AssetValidation"), NSLOCTEXT("Contexts", "AssetValidation", "AssetValidation Plugin"), NAME_None, FAssetValidationStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
};
