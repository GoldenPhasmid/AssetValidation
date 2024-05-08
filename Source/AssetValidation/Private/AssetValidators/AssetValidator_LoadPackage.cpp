#include "AssetValidators/AssetValidator_LoadPackage.h"

#include "AssetCompilingManager.h"
#include "AssetValidationModule.h"
#include "AssetValidationStatics.h"
#include "DataValidationModule.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"

bool UAssetValidator_LoadPackage::GetPackageLoadErrors(const FString& PackageName, TArray<FString>& OutWarnings, TArray<FString>& OutErrors)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(LoadPackage_GetErrors, AssetValidationChannel);
	
	check(GEngine);
	check(!PackageName.IsEmpty());

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
		UE::AssetValidation::FLogMessageGatherer Gatherer;
		Package = LoadPackage(nullptr, *PackageName, LOAD_None);

		OutWarnings.Append(Gatherer.GetWarnings());
		OutErrors.Append(Gatherer.GetErrors());
		return true;
	}

	static int32 PackageIdentifier = 0;
	const FString DestPackageName = FString::Printf(TEXT("/Temp/%s_%d"), *FPackageName::GetLongPackageAssetName(PackageName), ++PackageIdentifier);
	const FString DestFilename = FPackageName::LongPackageNameToFilename(DestPackageName, FPaths::GetExtension(SourceFilename, true));

	{
		TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(LoadPackage_CopyFile, AssetValidationChannel);
		const uint32 CopyResult = IFileManager::Get().Copy(*DestFilename, *SourceFilename);
		if (!ensure(CopyResult == COPY_OK))
		{
			UE_LOG(LogAssetValidation, Error, TEXT("Failed to copy package in editor, source [%s], destination [%s]"), *SourceFilename, *DestFilename);
			return false;
		}
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
		UE::AssetValidation::FLogMessageGatherer Gatherer;

		const int32 LoadFlags = LOAD_ForDiff | LOAD_DisableCompileOnLoad;
		UPackage* DestPackage = LoadPackage(nullptr, *DestPackageName, LoadFlags);

		FAssetCompilingManager::Get().FinishAllCompilation();
		
		if (DestPackage)
		{
			auto FixupAndCopyMessages = [&](const TArray<FString>& Input, TArray<FString>& Output)
			{
				for (FString Msg: Input)
				{
					Msg = Msg.Replace(*DestFilename, *SourceFilename);
					Msg = Msg.Replace(*DestPackageName, *PackageName);
					Output.Add(MoveTemp(Msg));
				}
			};
			// correct messages with real package name, remove the temp one
			FixupAndCopyMessages(Gatherer.GetWarnings(), OutWarnings);
			FixupAndCopyMessages(Gatherer.GetErrors(), OutErrors);
			
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

			// GEngine->ForceGarbageCollection(true);
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS, false);
		}
	}

	return true;
}

bool UAssetValidator_LoadPackage::IsEnabled() const
{
	// Commandlets do not need this validation step as they loaded the content while running.
	return false && !IsRunningCommandlet() && Super::IsEnabled();
}

bool UAssetValidator_LoadPackage::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	return Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext);
}

EDataValidationResult UAssetValidator_LoadPackage::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_LoadPackage, AssetValidationChannel);
	
	FString PackageName = InAssetData.PackageName.ToString();
	if (UNLIKELY(PackageName.IsEmpty()))
	{
		PackageName = InAsset ? InAsset->GetPackage()->GetName() : TEXT("");
	}
	
	TArray<FString> Warnings, Errors;
	if (GetPackageLoadErrors(InAssetData.PackageName.ToString(), Warnings, Errors))
	{
		for (const FString& Warning: Warnings)
		{
			AssetWarning(InAsset, FText::FromString(Warning));
		}

		for (const FString& Error: Errors)
		{
			AssetFails(InAsset, FText::FromString(Error));
		}
	}

	if (GetValidationResult() != EDataValidationResult::Invalid)
	{
		AssetPasses(InAsset);
	}
	
	return GetValidationResult();
}

