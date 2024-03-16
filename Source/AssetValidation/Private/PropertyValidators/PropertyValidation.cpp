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

void FPropertyValidationContext::PropertyFails(const FProperty* Property, const FText& DefaultFailureMessage)
{
	FProperty* OwnerProperty = Property->GetOwner<FProperty>();
	const bool bContainerProperty = UPropertyValidatorSubsystem::IsContainerProperty(OwnerProperty);

	FIssue Issue;
	Issue.IssueProperty = bContainerProperty ? OwnerProperty : Property;
	Issue.Severity = EMessageSeverity::Error;

	FText PropertyPrefix = FText::GetEmpty();
	if (!bContainerProperty)
	{
		PropertyPrefix = Property->GetDisplayNameText();
	}
	
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
	FString CorrectContext = ContextString;
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