#pragma once
#include "AssetValidationStatics.h"

enum class EDataValidationUsecase : uint8;

struct FDataValidationParameters
{
	EDataValidationUsecase ValidationUsecase;
	bool bInteractive;
};

struct FDataValidationResult
{
	EDataValidationResult Result;
	TArray<FString> Warnings;
	TArray<FString> Errors;
};

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

	static void ValidateChangelistContent(bool bInteractive, EDataValidationUsecase Usecase, TArray<FString>& OutWarnings, TArray<FString>& OutErrors);
	static void ValidatePackages(const TArray<FString>& PackagesToValidate, EDataValidationUsecase Usecase, TArray<FString>& OutWarnings, TArray<FString>& OutErrors);
	static bool ValidateProjectSettings(EDataValidationUsecase ValidationUsecase, TArray<FString>& OutWarnings, TArray<FString>& OutErrors);
	static FString ValidateEmptyPackage(const FString& PackageName);
};
