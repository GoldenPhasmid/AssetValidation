#pragma once

#include <mutex>

#include "AssetValidationStatics.h"
#include "AssetValidationDefines.h"

struct FValidateAssetsResults;
struct FValidateAssetsSettings;

enum class EDataValidationUsecase : uint8;

#if !WITH_DATA_VALIDATION_UPDATE
class UActorDescContainer;
template<class ActorDescContPtrType>
class TActorDescContainerCollection;

using FActorDescContainerCollection = TActorDescContainerCollection<TObjectPtr<UActorDescContainer>>;
#endif

namespace UE::AssetValidation
{

struct FAssetValidationMessageGatherer: public FOutputDevice
{
	FAssetValidationMessageGatherer()
		: FOutputDevice()
	{
		GLog->AddOutputDevice(this);
	}

	virtual ~FAssetValidationMessageGatherer() override
	{
		std::scoped_lock Lock{CriticalSection};
		GLog->RemoveOutputDevice(this);
	}

	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
	{
		std::scoped_lock Lock{CriticalSection};
		
		const FString Message(V);
		if (Verbosity == ELogVerbosity::Warning)
		{
			Warnings.Add(Message);
		}
		else if (Verbosity == ELogVerbosity::Error)
		{
			Errors.Add(Message);
		}
	}

	TArray<FString> GetErrors() const
	{
		return Errors;
	}

	TArray<FString> GetWarnings() const
	{
		return Warnings;
	}

	void Release(TArray<FString>& OutWarnings, TArray<FString>& OutErrors)
	{
		std::scoped_lock Lock{CriticalSection};
		OutWarnings.Append(Warnings);
		OutErrors.Append(Errors);
	}

private:
	std::mutex CriticalSection;
	
	TArray<FString> Warnings;
	TArray<FString> Errors;
};

// @todo: refactor
struct FLogMessageGatherer: public FAssetValidationMessageGatherer {};
	
} // UE::AssetValidation


namespace UE::AssetValidation
{
	/**
	 * Validate opened files under source control.
	 * Depending on source control provider it either validates active changelist (Perforce) or active changes (Git)
	 * @param bInteractive whether to show message dialogs or dump information to log
	 * @param InSettings validation settings
	 * @param OutResults validation results
	 * @return number of failures
	 */
	static int32 ValidateCheckedOutAssets(bool bInteractive, const FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults);

#if !WITH_DATA_VALIDATION_UPDATE // starting from 5.4 asset validation utilizes WP validators that previously worked for perforce only
	/**
	 * Validate OFPA packages and World Partition runtime settings
	 * Duplicates functionality from UWorldPartitionChangelistValidator because git doesn't have changelists :)
	 * @see UWorldPartitionChangelistValidator
	 */
	static void ValidateWorldPartitionActors(const TArray<FSourceControlStateRef>& FileStates, TArray<FString>& OutWarnings, TArray<FString>& OutErrors);

	/**
	 * Checks whether any source controlled files are unsaved in editor (have dirty packages)
	 * @note Duplicates functionality from UDirtyFilesChangelistValidator because git doesn't have changelists :)
	 * @param FileStates
	 * @param OutWarnings
	 * @param OutErrors
	 * @see UDirtyFilesChangelistValidator
	 */
	static void ValidateDirtyFiles(const TArray<FSourceControlStateRef>& FileStates, TArray<FString>& OutWarnings, TArray<FString>& OutErrors);
#endif
	
	/**
	 * Run asset validation for packages that were modified under source control
	 * @param ModifiedPackages
	 * @param DeletedPackages
	 * @param InSettings data validation settings
	 * @param OutResults data validation results
	 */
	static void ValidatePackages(TConstArrayView<FString> ModifiedPackages, TConstArrayView<FString> DeletedPackages, const FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults);

	static bool ShouldValidatePackage(const FString& PackageName);
	/**
	 * Validates project settings.
	 * @param Context data validation context
	 */
	static void ValidateProjectSettings(const FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults);

	/** validate source control file state for an empty package */
	static FString ValidateEmptyPackage(const FString& PackageName);

	/** Given a package, return if package contains a UWorld or an external world object */
	static bool IsWorldOrWorldExternalPackage(UPackage* Package);

#if !WITH_DATA_VALIDATION_UPDATE // starting from 5.4 asset validation utilizes WP validators that previously worked for perforce only
	static FString GetPackagePath(const UPackage* Package);

	static UClass* GetAssetNativeClass(const FAssetData& AssetData);
	static FTopLevelAssetPath GetAssetWorld(const FAssetData& AssetData);

	static void RegisterActorContainer(UWorld* World, FName ContainerPackageName, FActorDescContainerCollection& RegisteredContainers);
#endif

	/** @return true if filename is a C++ source file */
	static bool IsCppFile(const FString& Filename);

	/** Add validation messages to validation context in "data validation format" */
	static void AppendValidationMessages(FDataValidationContext& ValidationContext, const FAssetData& AssetData, UE::AssetValidation::FLogMessageGatherer& Gatherer);
	static void AppendValidationMessages(FDataValidationContext& ValidationContext, const FAssetData& AssetData, EMessageSeverity::Type Severity, TConstArrayView<FString> Messages);
} // UE::AssetValidation
