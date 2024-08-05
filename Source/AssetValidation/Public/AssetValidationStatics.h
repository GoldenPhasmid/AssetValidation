#pragma once

#include <mutex>

namespace UE::DataValidation
{
	struct FScopedLogMessageGatherer;
}

class UEditorValidatorBase;
struct FValidateAssetsResults;
struct FValidateAssetsSettings;
struct FAssetData;

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
	ASSETVALIDATION_API void ValidatePackages(TConstArrayView<FString> ModifiedPackages, TConstArrayView<FString> DeletedPackages, const FValidateAssetsSettings& InSettings, FValidateAssetsResults& OutResults);

	/** @return true if assets defined by @PackageName should be validated */
	ASSETVALIDATION_API bool ShouldValidatePackage(const FString& PackageName);

	/** Enable or disable a given editor validator */
	void SetValidatorEnabled(UEditorValidatorBase* Validator, bool bEnabled);

	/** validate source control file state for an empty package */
	FString ValidateEmptyPackage(const FString& PackageName);

	/** Given a package, return if package contains a UWorld or an external world object */
	bool IsWorldOrWorldExternalPackage(UPackage* Package);

	/** @return whether asset data represents a world asset */
	bool IsWorldAsset(const FAssetData& AssetData);

	/** @return whether package path represents external actor */
	bool IsExternalAsset(const FString& PackagePath);
	
	/** @return whether asset data represents external actor */
	bool IsExternalAsset(const FAssetData& AssetData);

	/** @return true if filename is a C++ source file */
	bool IsCppFile(const FString& Filename);
	
	template <typename T>
	TSharedRef<FTokenizedMessage> AddToken(const TSharedRef<FTokenizedMessage>& Message, const T& Token);
	
	/** @return tokenized message */
	template <typename ...TParams>
	TSharedRef<FTokenizedMessage> CreateTokenMessage(EMessageSeverity::Type Severity, TParams&&... Params)
	{
		TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(Severity);
		(AddToken<std::decay_t<TParams>>(Message, Params), ...);

		return Message;
	}
	
	/** Add validation messages to validation context in "data validation format" */
	ASSETVALIDATION_API void ClearLogMessages(FMessageLog& MessageLog);
	ASSETVALIDATION_API void AppendAssetValidationMessages(FMessageLog& MessageLog, FDataValidationContext& ValidationContext);
	ASSETVALIDATION_API void AppendAssetValidationMessages(FMessageLog& MessageLog, const FAssetData& AssetData, FDataValidationContext& ValidationContext);
	ASSETVALIDATION_API void AppendAssetValidationMessages(FDataValidationContext& ValidationContext, const FAssetData& AssetData, UE::DataValidation::FScopedLogMessageGatherer& Gatherer);
	ASSETVALIDATION_API void AppendAssetValidationMessages(FDataValidationContext& ValidationContext, const FAssetData& AssetData, EMessageSeverity::Type Severity, TConstArrayView<FText> Messages);
	ASSETVALIDATION_API void AppendAssetValidationMessages(FDataValidationContext& ValidationContext, const FAssetData& AssetData, EMessageSeverity::Type Severity, TConstArrayView<FString> Messages);
} // UE::AssetValidation
