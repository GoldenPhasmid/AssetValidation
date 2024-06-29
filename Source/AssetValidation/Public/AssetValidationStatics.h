#pragma once

#include <mutex>

#include "AssetValidationDefines.h"
#include "EditorValidatorHelpers.h"
#include "Misc/DataValidation.h"

struct FValidateAssetsResults;
struct FValidateAssetsSettings;

enum class EDataValidationUsecase : uint8;

namespace UE::AssetValidation
{

struct FScopedLogMessageGatherer: public FOutputDevice
{
	FScopedLogMessageGatherer(const FAssetData& InAssetData, FDataValidationContext& InContext);
	FScopedLogMessageGatherer(const FAssetData& InAssetData, FDataValidationContext& InContext, TFunction<FString(const FString&)> InLogConverter);

	virtual ~FScopedLogMessageGatherer() override;

	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override;

private:
	std::recursive_mutex CriticalSection;
	FAssetData AssetData;
	FDataValidationContext& Context;
	TFunction<FString(const FString&)> LogConverter;
};
	
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
	ASSETVALIDATION_API int32 ValidateCheckedOutAssets(bool bInteractive, const FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults);

	/**
	 * Validates project settings.
	 * @param InSettings data validation context
	 * @param OutResults
	 */
	ASSETVALIDATION_API void ValidateProjectSettings(const FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults);
	
	/**
	 * Run asset validation for packages that were modified under source control
	 * @param ModifiedPackages
	 * @param DeletedPackages
	 * @param InSettings data validation settings
	 * @param OutResults data validation results
	 */
	void ValidatePackages(TConstArrayView<FString> ModifiedPackages, TConstArrayView<FString> DeletedPackages, const FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults);

	bool ShouldValidatePackage(const FString& PackageName);

	/** validate source control file state for an empty package */
	FString ValidateEmptyPackage(const FString& PackageName);

	/** Given a package, return if package contains a UWorld or an external world object */
	bool IsWorldOrWorldExternalPackage(UPackage* Package);

	/** @return true if filename is a C++ source file */
	bool IsCppFile(const FString& Filename);

	/** Add validation messages to validation context in "data validation format" */
	ASSETVALIDATION_API void AppendAssetValidationMessages(FMessageLog& MessageLog, FDataValidationContext& ValidationContext);
	ASSETVALIDATION_API void AppendAssetValidationMessages(FMessageLog& MessageLog, const FAssetData& AssetData, FDataValidationContext& ValidationContext);
	ASSETVALIDATION_API void AppendAssetValidationMessages(FDataValidationContext& ValidationContext, const FAssetData& AssetData, UE::DataValidation::FScopedLogMessageGatherer& Gatherer);
	ASSETVALIDATION_API void AppendAssetValidationMessages(FDataValidationContext& ValidationContext, const FAssetData& AssetData, EMessageSeverity::Type Severity, TConstArrayView<FText> Messages);
	ASSETVALIDATION_API void AppendAssetValidationMessages(FDataValidationContext& ValidationContext, const FAssetData& AssetData, EMessageSeverity::Type Severity, TConstArrayView<FString> Messages);
} // UE::AssetValidation
