#pragma once
#include "AssetValidationStatics.h"
#include "ISourceControlState.h"

class UActorDescContainer;
enum class EDataValidationUsecase : uint8;

template<class ActorDescContPtrType>
class TActorDescContainerCollection;

using FActorDescContainerCollection = TActorDescContainerCollection<TObjectPtr<UActorDescContainer>>;

struct FLogMessageGatherer: public FOutputDevice
{
	FLogMessageGatherer()
		: FOutputDevice()
	{
		GLog->AddOutputDevice(this);
	}

	virtual ~FLogMessageGatherer() override
	{
		GLog->RemoveOutputDevice(this);
	}

	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
	{
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

private:
	TArray<FString> Warnings;
	TArray<FString> Errors;
};


class AssetValidationStatics
{
public:
	/**
	 * Validate all files under source control
	 * @param bInteractive whether to show message dialogs or dump information to log
	 * @param Usecase validation use case
	 * @param OutWarnings validation warnings
	 * @param OutErrors validation errors
	 */
	static void ValidateSourceControl(bool bInteractive, EDataValidationUsecase Usecase, TArray<FString>& OutWarnings, TArray<FString>& OutErrors);

	/**
	 * Validate OFPA packages and World Partition runtime settings
	 * Duplicates functionality from UWorldPartitionChangelistValidator because git doesn't have changelists :)
	 * @see UWorldPartitionChangelistValidator
	 */
	static void ValidateWorldPartitionActors(const TArray<FSourceControlStateRef>& FileStates, TArray<FString>& OutWarnings, TArray<FString>& OutErrors);

	/**
	 * Checks whether any source controlled files are unsaved in editor (have dirty packages)
	 * Duplicates functionality from UDirtyFilesChangelistValidator because git doesn't have changelists :)
	 * @see UDirtyFilesChangelistValidator
	 */
	static void ValidateDirtyFiles(const TArray<FSourceControlStateRef>& FileStates, TArray<FString>& OutWarnings, TArray<FString>& OutErrors);
	static void ValidatePackages(const TArray<FString>& PackagesToValidate, EDataValidationUsecase Usecase, TArray<FString>& OutWarnings, TArray<FString>& OutErrors);

	/**
	 * Validates project settings.
	 * @return whether found any validation errors
	 */
	static bool ValidateProjectSettings(EDataValidationUsecase ValidationUsecase, TArray<FString>& OutWarnings, TArray<FString>& OutErrors);

	static FString ValidateEmptyPackage(const FString& PackageName);
	static FString GetPackagePath(const UPackage* Package);

	static UClass* GetAssetNativeClass(const FAssetData& AssetData);
	static FTopLevelAssetPath GetAssetWorld(const FAssetData& AssetData);

	static void RegisterActorContainer(UWorld* World, FName ContainerPackageName, FActorDescContainerCollection& RegisteredContainers);
};
