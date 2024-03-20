#pragma once

struct ASSETVALIDATION_API FPropertyValidationResult
{
	FPropertyValidationResult() = default;
	FPropertyValidationResult(EDataValidationResult InResult)
		: ValidationResult(InResult)
	{ }
	
	TArray<FText> Errors;
	TArray<FText> Warnings;
	EDataValidationResult ValidationResult = EDataValidationResult::NotValidated;
};