#pragma once

#include <mutex>

#include "CoreMinimal.h"
#include "Misc/DataValidation.h"

class IAssetRegistry;

class UEditorValidatorBase;
struct FValidateAssetsResults;
struct FValidateAssetsSettings;
struct FAssetData;

enum class EDataValidationUsecase : uint8;

namespace UE::AssetValidation
{

/**
 * While in scope, any messages received by this output device automatically have asset info added to the tokenized message
 * @InAssetData asset data
 * @InContext validation context
 * @InLogConverter additional message post processing
 */
struct FScopedAssetContext: public FOutputDevice
{
	FScopedAssetContext(const FAssetData& InAssetData, FDataValidationContext& InContext);
	FScopedAssetContext(const FAssetData& InAssetData, FDataValidationContext& InContext, TFunction<FString(const FString&)> InLogConverter);

	virtual ~FScopedAssetContext() override;

	virtual bool CanBeUsedOnMultipleThreads() const override { return true; }
	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override;

private:
	std::recursive_mutex CriticalSection;
	FAssetData AssetData;
	FDataValidationContext& Context;
	TFunction<FString(const FString&)> LogConverter;
};

/**
 * While in scope, any messages received by this output device 
 */
struct FScopedLogMessageGatherer: public FOutputDevice
{
	explicit FScopedLogMessageGatherer(bool bInEnabled);
	virtual ~FScopedLogMessageGatherer() override;

	virtual bool CanBeUsedOnMultipleThreads() const override { return true; }
	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override;

	void Stop(TArray<FString>& OutWarnings, TArray<FString>& OutErrors);

private:
	std::recursive_mutex CriticalSection;
	TArray<FString> Warnings;
	TArray<FString> Errors;
	bool bEnabled = false;
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

	/** @return memory and disk size for a given asset */
	bool GetAssetSizeBytes(IAssetRegistry& AssetRegistry, const FAssetData& AssetData, float& OutMemorySize, float& OutDiskSize);

	/** @return first C++ defined base class */
	UClass* GetCppBaseClass(UClass* InClass);

	template <typename T, typename = void>
	struct TConvertTokenImpl
	{
		using Type = T;
		static T Convert(T Value) { return Value; }
	};

	template <bool Value>
	struct TOnlyTrue;

	template <>
	struct TOnlyTrue<true> {};

	template <typename TMessageToken, ESPMode Mode>
	struct TConvertTokenImpl<TSharedPtr<TMessageToken, Mode>, std::void_t<TOnlyTrue<std::is_convertible_v<TMessageToken, IMessageToken>>>>
	{
		using Type = TSharedRef<IMessageToken, Mode>;
		static TSharedRef<IMessageToken, Mode> Convert(const TSharedPtr<TMessageToken>& Token)
		{
			return StaticCastSharedPtr<IMessageToken>(Token).ToSharedRef();
		}
	};

	template <typename TMessageToken, ESPMode Mode>
	struct TConvertTokenImpl<TSharedRef<TMessageToken, Mode>, std::void_t<TOnlyTrue<std::is_convertible_v<TMessageToken, IMessageToken>>>>
	{
		using Type = TSharedRef<IMessageToken, Mode>;
		static TSharedRef<IMessageToken, Mode> Convert(const TSharedRef<TMessageToken>& Token)
		{
			return StaticCastSharedRef<IMessageToken>(Token);
		}
	};

namespace Private
{
	template <typename T>
	struct TConvertToken: TConvertTokenImpl<std::decay_t<T>> {};

	template <typename T>
	using TConvertTokenType = typename TConvertToken<T>::Type;

	template <typename T>
	TSharedRef<FTokenizedMessage> AddToken(const TSharedRef<FTokenizedMessage>& Message, const T& Token);

	template <typename T>
	TSharedRef<FTokenizedMessage> AddToken(const TSharedRef<FTokenizedMessage>& Message, const T* Token);
} // Private
	
	/** create and @return tokenized message */
	template <typename ...TParams>
	TSharedRef<FTokenizedMessage> CreateTokenMessage(EMessageSeverity::Type Severity, TParams&&... Params)
	{
		TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(Severity);
		(Private::AddToken<Private::TConvertTokenType<TParams>>(Message, Private::TConvertToken<TParams>::Convert(Params)), ...);

		return Message;
	}

	/** create and add tokenized message to the data validation context */
	template <typename ...TParams>
	void AddTokenMessage(FDataValidationContext& Context, EMessageSeverity::Type Severity, TParams&&... Params)
	{
		TSharedRef<FTokenizedMessage> Message = FTokenizedMessage::Create(Severity);
		(Private::AddToken<Private::TConvertTokenType<TParams>>(Message, Private::TConvertToken<TParams>::Convert(Params)), ...);

		Context.AddMessage(Message);
	}
	
	/** Clear log listing defined by the message log */
	ASSETVALIDATION_API void ClearLogMessages(FMessageLog& MessageLog);
	/** Transfer gathered warnings and errors from Data Validation context to the message log, in "data validation format" */
	ASSETVALIDATION_API void AppendMessages(FMessageLog& MessageLog, FDataValidationContext& ValidationContext);
	ASSETVALIDATION_API void AppendMessages(FMessageLog& MessageLog, const FAssetData& AssetData, FDataValidationContext& ValidationContext);
	/** Transfer gathered messages by the scoped log and transfer them to the Data Validation context */
	ASSETVALIDATION_API void AppendMessages(FDataValidationContext& ValidationContext, const FAssetData& AssetData, FScopedLogMessageGatherer& Gatherer);
	/** Add a list of messages related to the @AssetData with a specified @Severity */
	ASSETVALIDATION_API void AppendMessages(FDataValidationContext& ValidationContext, const FAssetData& AssetData, EMessageSeverity::Type Severity, TConstArrayView<FText> Messages);
	ASSETVALIDATION_API void AppendMessages(FDataValidationContext& ValidationContext, const FAssetData& AssetData, EMessageSeverity::Type Severity, TConstArrayView<FString> Messages);
} // UE::AssetValidation
