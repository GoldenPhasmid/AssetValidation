#include "AssetValidators/AssetValidator_LoadPackage.h"

#include "AssetCompilingManager.h"
#include "AssetValidationModule.h"
#include "AssetValidationStatics.h"

bool UAssetValidator_LoadPackage::GetPackageLoadErrors(const FString& PackageName, TArray<FString>& OutWarnings, TArray<FString>& OutErrors)
{
	check(GEngine);
	check(!PackageName.IsEmpty());

	UPackage* Package = FindPackage(nullptr, *PackageName);
	if (Package == GetTransientPackage())
	{
		return true;
	}

	if (Package && UWorld::IsWorldOrExternalActorPackage(Package))
	{
		// don't validate external actors
		return true;
	}

	if (Package && (Package->ContainsMap() || Package->HasAnyPackageFlags(PKG_ContainsMapData) || PackageName.EndsWith("_BuildData")))
	{
		// don't validate map packages
		return true;
	}

	FString SourceFilename;
	if (!FPackageName::DoesPackageExist(PackageName, &SourceFilename))
	{
		if (!PackageName.StartsWith(TEXT("/Script/")))
		{
			// in memory but not yet saved, and its not a script package 
			UE_LOG(LogAssetValidation, Warning, TEXT("Package %s is in memory but not yet saved (no source file)"), *PackageName);
		}

		// otherwise package is a script package and we're ok
		return false;
	}

	if (Package == nullptr)
	{
		// Not in memory, just load it
		FLogMessageGatherer Gatherer;
		Package = LoadPackage(nullptr, *PackageName, LOAD_None);

		OutWarnings = Gatherer.GetWarnings();
		OutErrors = Gatherer.GetErrors();
		return true;
	}

	static int32 PackageIdentifier = 0;
	const FString DestPackageName = FString::Printf(TEXT("/Temp/%s_%d"), *FPackageName::GetLongPackageAssetName(PackageName), ++PackageIdentifier);
	const FString DestFilename = FPackageName::LongPackageNameToFilename(DestPackageName, FPaths::GetExtension(SourceFilename, true));

	const uint32 CopyResult = IFileManager::Get().Copy(*DestFilename, *SourceFilename);
	if (!ensure(CopyResult == COPY_OK))
	{
		UE_LOG(LogAssetValidation, Error, TEXT("Failed to copy package in editor, source [%s], destination [%s]"), *SourceFilename, *DestFilename);
		return false;
	}

	{
		FLogMessageGatherer Gatherer;

		const int32 LoadFlags = LOAD_ForDiff | LOAD_DisableCompileOnLoad;
		UPackage* DestPackage = LoadPackage(nullptr, *DestPackageName, LoadFlags);

		FAssetCompilingManager::Get().FinishAllCompilation();
		
		if (DestPackage)
		{
			auto CopyMessages = [&](const TArray<FString>& Input, TArray<FString>& Output)
			{
				for (FString Msg: Input)
				{
					Msg = Msg.Replace(*DestFilename, *SourceFilename);
					Msg = Msg.Replace(*DestPackageName, *PackageName);
					Output.Add(MoveTemp(Msg));
				}
			};
			// correct messages with real package name, remove the temp one
			CopyMessages(Gatherer.GetWarnings(), OutWarnings);
			CopyMessages(Gatherer.GetErrors(), OutErrors);
			
			ResetLoaders(DestPackage);
			IFileManager::Get().Delete(*DestFilename);

			// remove temporary objects from memory. Mark everything as garbage and force garbage collection
			// Otherwise packages will duplicate in editor
			TArray<UObject*> TempLoadedObjects;
			GetObjectsWithPackage(DestPackage, TempLoadedObjects, true);

			for (UObject* TempObject: TempLoadedObjects)
			{
				if (TempObject->IsRooted())
				{
					check(false);
					continue;;
				}
				
				TempObject->ClearFlags(RF_Public | RF_Standalone);
				TempObject->SetFlags(RF_Transient);

				if (UWorld* TempWorld = Cast<UWorld>(TempObject))
				{
					TempWorld->DestroyWorld(true);
				}
				TempObject->MarkAsGarbage();
			}
			
			GEngine->ForceGarbageCollection(true);
		}

		
	}

	return true;
}

bool UAssetValidator_LoadPackage::IsEnabled() const
{
	// Commandlets do not need this validation step as they loaded the content while running.
	return !IsRunningCommandlet() && Super::IsEnabled();
}

bool UAssetValidator_LoadPackage::CanValidate_Implementation(const EDataValidationUsecase InUsecase) const
{
	return true;
}

EDataValidationResult UAssetValidator_LoadPackage::ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors)
{
	check(InAsset);

	TArray<FString> Warnings, Errors;
	if (GetPackageLoadErrors(InAsset->GetPackage()->GetName(), Warnings, Errors))
	{
		for (const FString& Warning: Warnings)
		{
			AssetWarning(InAsset, FText::FromString(Warning));
		}

		for (const FString& Error: Errors)
		{
			AssetFails(InAsset, FText::FromString(Error), ValidationErrors);
		}
	}

	if (GetValidationResult() != EDataValidationResult::Invalid)
	{
		AssetPasses(InAsset);
	}
	
	return GetValidationResult();
}
