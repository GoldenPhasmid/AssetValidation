#include "PropertyValidation.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

FPropertyValidationResult FPropertyValidationContext::MakeValidationResult() const
{
	FPropertyValidationResult Result;
	Result.Errors.Reserve(Issues.Num());
	Result.Warnings.Reserve(Issues.Num());
	
	for (const FIssue& Issue: Issues)
	{
		switch (Issue.Severity)
		{
		case EMessageSeverity::Error:
			Result.Errors.Add(Issue.Message);
			break;
		case EMessageSeverity::Warning:
			Result.Warnings.Add(Issue.Message);
		default:
			checkNoEntry();
		}
	}
	Result.ValidationResult = Result.Errors.IsEmpty() ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
	return Result;
}

void FPropertyValidationContext::PropertyFails(FProperty* Property, const FText& DefaultFailureMessage)
{
	FIssue Issue;
	Issue.IssueProperty = Property;
	
	if (const FString* CustomMsg = Property->FindMetaData(ValidationNames::ValidationFailureMessage))
	{
		Issue.Message = MakeFullMessage(FText::FromString(*CustomMsg));
	}
	else
	{
		Issue.Message = MakeFullMessage(DefaultFailureMessage);
	}

	if (Property->HasMetaData(ValidationNames::Validate))
	{
		Issue.Severity = EMessageSeverity::Error;
	}
	else if (Property->HasMetaData(ValidationNames::ValidateWarning))
	{
		Issue.Severity = EMessageSeverity::Warning;
	}
	else
	{
		checkNoEntry();
	}

	Issues.Add(Issue);
}

FText FPropertyValidationContext::MakeFullMessage(const FText& FailureMessage) const
{
	// remove last dot
	const FString CorrectContext = Context.RightChop(1);
	
	FFormatNamedArguments NamedArguments;
	NamedArguments.Add(TEXT("Context"), FText::FromString(CorrectContext));
	NamedArguments.Add(TEXT("FailureMessage"), FailureMessage);

	return FText::Format(FTextFormat(LOCTEXT("AssetValidation_Message", "{Context}: {FailureMessage}")), NamedArguments);
}

#undef LOCTEXT_NAMESPACE