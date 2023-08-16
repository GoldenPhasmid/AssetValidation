// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetValidators/AssetValidator.h"

#include "Settings/ProjectPackagingSettings.h"

bool UAssetValidator::CanValidateAsset_Implementation(UObject* InAsset) const
{
	const FString PackageName = InAsset->GetPackage()->GetName();
	
	const UProjectPackagingSettings* PackagingSettings = GetDefault<UProjectPackagingSettings>();

	for (const FDirectoryPath& Directory: PackagingSettings->DirectoriesToNeverCook)
	{
		const FString& Folder = Directory.Path;
		if (PackageName.StartsWith(Folder))
		{
			return false;
		}
	}

	if (PackageName.StartsWith(TEXT("/Game/Developers/")))
	{
		return false;
	}

	return true;
}
