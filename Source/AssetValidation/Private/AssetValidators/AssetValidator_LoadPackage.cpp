#include "AssetValidators/AssetValidator_LoadPackage.h"

#include "AssetCompilingManager.h"
#include "AssetValidationStatics.h"
#include "AssetValidationSubsystem.h"

bool UAssetValidator_LoadPackage::GetPackageLoadErrors(const FString& PackageName, const FAssetData& AssetData, FDataValidationContext& ValidationContext)
{
	check(!PackageName.IsEmpty());
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(GetPackageLoadErrors, AssetValidationChannel);
	
	if (UAssetValidationSubsystem::IsPackageAlreadyLoaded(FName{PackageName}))
	{
		return true;
	}

	UAssetValidationSubsystem::MarkPackageLoaded(FName{PackageName});

	UPackage* Package = FindPackage(nullptr, *PackageName);
	if (Package == GetTransientPackage())
	{
		return true;
	}
	
	if (Package && UE::AssetValidation::IsWorldOrWorldExternalPackage(Package))
	{
		// don't validate world packages or external actors
		return true;
	}

	if (Package && (Package->ContainsMap() || Package->HasAnyPackageFlags(PKG_ContainsMapData) || PackageName.EndsWith("_BuildData")))
	{
		// don't validate map packages, build data assets or any map data
		return true;
	}

	FString SourceFilename;
	if (!FPackageName::DoesPackageExist(PackageName, &SourceFilename))
	{
		if (!FPackageName::IsScriptPackage(PackageName))
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
		UE::AssetValidation::FScopedLogMessageGatherer Gatherer{AssetData, ValidationContext};
		Package = LoadPackage(nullptr, *PackageName, LOAD_None);
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

	// @todo: this crashes because GetObjectsWithPackage returns some garbage object as a last element
	// and it is impossible to verify that it is a garbage (because flags are normal and it is a part of root set?)
	// crash happens in Cast, skipping through null check and low level validity check
#if 0
	TArray<UObject*> LoadedObjects;
	GetObjectsWithPackage(Package, LoadedObjects, false);
	
	// compile all loaded blueprints from existing package
	for (UObject* LoadedObject: LoadedObjects)
	{
		if (LoadedObject && LoadedObject->IsValidLowLevel())
		{
			if (UBlueprint* Blueprint = Cast<UBlueprint>(LoadedObject);
				Blueprint && !FBlueprintEditorUtils::IsDataOnlyBlueprint(Blueprint))
			{
				FKismetEditorUtilities::CompileBlueprint(Blueprint);
			}
		}
	}
#endif
	
	{
		auto ConvertMessage = [&DestFilename, &DestPackageName, &SourceFilename, &PackageName](const FString& Message)
		{
			// correct messages with real package name, remove the temp one
			FString Msg{Message};
			Msg = Msg.Replace(*DestFilename, *SourceFilename);
			Msg = Msg.Replace(*DestPackageName, *PackageName);

			return MoveTemp(Msg);
		};
		UE::AssetValidation::FScopedLogMessageGatherer Gatherer{AssetData, ValidationContext, ConvertMessage};

		const int32 LoadFlags = LOAD_ForDiff | LOAD_DisableCompileOnLoad;
		UPackage* DestPackage = LoadPackage(nullptr, *DestPackageName, LoadFlags);

		FAssetCompilingManager::Get().FinishAllCompilation();
		
		if (DestPackage)
		{
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
					continue;
				}
				
				TempObject->ClearFlags(RF_Public | RF_Standalone);
				TempObject->SetFlags(RF_Transient);

				if (UWorld* TempWorld = Cast<UWorld>(TempObject))
				{
					TempWorld->DestroyWorld(true);
				}
				TempObject->MarkAsGarbage();
			}
		}
	}

	return true;
}

UAssetValidator_LoadPackage::UAssetValidator_LoadPackage()
{
	AllowedContext &= ~EAssetValidationFlags::Save;
	AllowedContext &= ~EAssetValidationFlags::Commandlet;
	// if asset is loaded, we do a copy and check for load errors
	// if asset is unloaded, we load it instead
	bAllowNullAsset = true;
}

bool UAssetValidator_LoadPackage::IsEnabled() const
{
	// Commandlets do not need this validation step as they loaded the content while running.
	return !IsRunningCommandlet() && Super::IsEnabled();
}

EDataValidationResult UAssetValidator_LoadPackage::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_LoadPackage, AssetValidationChannel);
	
	FString PackageName = InAssetData.PackageName.ToString();
	if (UNLIKELY(PackageName.IsEmpty()))
	{
		PackageName = InAsset ? InAsset->GetPackage()->GetName() : TEXT("");
	}

	const uint32 NumErrors = Context.GetNumErrors();
	if (GetPackageLoadErrors(PackageName, InAssetData, Context))
	{
		if (Context.GetNumErrors() > NumErrors)
		{
			return EDataValidationResult::Invalid;
		}
	}

	AssetPasses(InAsset);
	return  EDataValidationResult::Valid;
}

