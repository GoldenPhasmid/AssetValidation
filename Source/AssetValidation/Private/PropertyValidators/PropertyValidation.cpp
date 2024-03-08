#include "PropertyValidators/PropertyValidation.h"

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

void FPropertyValidationContext::PropertyFails(const FProperty* Property, const FText& DefaultFailureMessage, const FText& PropertyPrefix)
{
	FIssue Issue;
	Issue.IssueProperty = Property;
	// @todo: warning validation
	Issue.Severity = EMessageSeverity::Error;
	
	if (const FString* CustomMsg = Property->FindMetaData(ValidationNames::ValidationFailureMessage))
	{
		Issue.Message = MakeFullMessage(FText::FromString(*CustomMsg), PropertyPrefix);
	}
	else
	{
		Issue.Message = MakeFullMessage(DefaultFailureMessage, PropertyPrefix);
	}

	Issues.Add(Issue);
}

FText FPropertyValidationContext::MakeFullMessage(const FText& FailureMessage, const FText& PropertyPrefix) const
{
	FString CorrectContext = Context;
	if (PropertyPrefix.IsEmpty())
	{
		// remove last dot
		CorrectContext = CorrectContext.LeftChop(1);
	}
	else
	{
		// append property prefix
		CorrectContext += PropertyPrefix.ToString();
	}
	
	FFormatNamedArguments NamedArguments;
	NamedArguments.Add(TEXT("Context"), FText::FromString(CorrectContext));
	NamedArguments.Add(TEXT("FailureMessage"), FailureMessage);

	return FText::Format(FTextFormat(LOCTEXT("AssetValidation_Message", "{Context}: {FailureMessage}")), NamedArguments);
}

#undef LOCTEXT_NAMESPACE