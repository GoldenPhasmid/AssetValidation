#include "AssetValidators/AssetValidator_AnimationAsset.h"

#include "AssetValidationDefines.h"
#include "AssetValidationStatics.h"
#include "PropertyValidatorSubsystem.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

UAssetValidator_AnimationAsset::UAssetValidator_AnimationAsset()
{
	bIsConfigDisabled = false; // enabled by default

	bCanRunParallelMode = true;
	bRequiresLoadedAsset = true;
	bRequiresTopLevelAsset = true;
	bCanValidateActors = false;
}

bool UAssetValidator_AnimationAsset::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	if (!Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext))
	{
		return false;
	}

	static struct FDerivedClasses
	{
		FDerivedClasses(const UClass* BaseClass)
		{
			TArray<UClass*> Classes;
			GetDerivedClasses(BaseClass, Classes);

			for (const UClass* Class: Classes)
			{
				ClassPaths.Add(Class->GetClassPathName());
			}
		}
		
		const TArray<FTopLevelAssetPath>& Get()
		{
			return ClassPaths;
		}

		TArray<FTopLevelAssetPath> ClassPaths;
	} Classes{UAnimationAsset::StaticClass()};

	return InObject != nullptr && Classes.Get().Contains(InAssetData.AssetClassPath);
}

EDataValidationResult UAssetValidator_AnimationAsset::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_AnimationAsset, AssetValidationChannel);

	UAnimationAsset* AnimationAsset = CastChecked<UAnimationAsset>(InAsset);
	const USkeleton* BaseSkeleton = AnimationAsset->GetSkeleton();
	
	EDataValidationResult Result = EDataValidationResult::Valid;
	
	Result &= ValidateSkeleton(BaseSkeleton, *AnimationAsset, InAssetData, Context);
	Result &= ValidateAnimNotifies(*AnimationAsset, InAssetData, Context);
	Result &= ValidateCurves(*AnimationAsset, InAssetData, Context);
	
	TArray<UAnimationAsset*> AnimationAssets;
	AnimationAsset->GetAllAnimationSequencesReferred(AnimationAssets);
	
	for (const UAnimationAsset* AnimAsset: AnimationAssets)
	{
		Result &= ValidateSkeleton(BaseSkeleton, *AnimAsset, InAssetData, Context);
		Result &= ValidateAnimNotifies(*AnimAsset, InAssetData, Context);
		Result &= ValidateCurves(*AnimAsset, InAssetData, Context);
	}

	if (Result != EDataValidationResult::Invalid)
	{
		AssetPasses(AnimationAsset);
	}

	return Result;
}

EDataValidationResult UAssetValidator_AnimationAsset::ValidateSkeleton(const USkeleton* Skeleton, const UAnimationAsset& AnimAsset, const FAssetData& AssetData, FDataValidationContext& Context)
{
	const USkeleton* AnimSkeleton = AnimAsset.GetSkeleton();
	if (!IsValid(AnimSkeleton))
	{
		UE::AssetValidation::AddTokenMessage(Context, EMessageSeverity::Error,
			AssetData, LOCTEXT("NoSkeleton", "Animation asset is missing skeleton.")
		);
		return EDataValidationResult::Invalid;
	}

	if (AnimSkeleton != Skeleton)
	{
		UE::AssetValidation::AddTokenMessage(Context, EMessageSeverity::Error, AssetData, FString{" "}, FAssetData{&AnimAsset},
			FText::Format(LOCTEXT("SkeletonMismatch", "Animation asset has a different skeleton. Expected [{0}], actual [{1}]"),
				FText::FromString(Skeleton->GetName()), FText::FromString(AnimSkeleton->GetName()))
		);
		return EDataValidationResult::Invalid;
	}

	return EDataValidationResult::Valid;
}

EDataValidationResult UAssetValidator_AnimationAsset::ValidateAnimNotifies(const UAnimationAsset& AnimAsset, const FAssetData& AssetData, FDataValidationContext& Context)
{
	const UAnimSequenceBase* AnimSequence = Cast<UAnimSequenceBase>(&AnimAsset);
	if (AnimSequence == nullptr)
	{
		return EDataValidationResult::Valid;
	}

	UPropertyValidatorSubsystem* PropertyValidators = GEditor->GetEditorSubsystem<UPropertyValidatorSubsystem>();
	
	FAssetData AnimSequenceAsset{AnimSequence};
	EDataValidationResult Result = EDataValidationResult::Valid;
	
	for (const FAnimNotifyEvent& AnimNotify: AnimSequence->Notifies)
	{
		if (AnimNotify.Notify == nullptr && AnimNotify.NotifyStateClass == nullptr)
		{
			const FText NotifyName = FText::FromName(AnimNotify.GetNotifyEventName());
			const float TriggerTime = AnimNotify.GetTriggerTime();
			const float TriggerTimePretty = static_cast<int32>(TriggerTime) + static_cast<int32>(FMath::Frac(TriggerTime) * 100) / 100.0;
			
			const FText FailReason = FText::Format(LOCTEXT("InvalidAnimNotify", "Anim notify event [{0}:Time {1}] was deleted and no longer valid. Remove it to fix this error."),
				NotifyName, FText::FromString(FString::SanitizeFloat(TriggerTimePretty, 2))
			);

			if (AssetData == AnimSequenceAsset)
			{
				UE::AssetValidation::AddTokenMessage(Context, EMessageSeverity::Error, AssetData, FailReason);
			}
			else
			{
				UE::AssetValidation::AddTokenMessage(Context, EMessageSeverity::Error, AssetData, FString{" "}, AnimSequenceAsset, FailReason);
			}

			Result &= EDataValidationResult::Invalid;
		}
		else
		{
			UObject* Target = AnimNotify.Notify ? static_cast<UObject*>(AnimNotify.Notify) : static_cast<UObject*>(AnimNotify.NotifyStateClass);
			FPropertyValidationResult ValidationResult = PropertyValidators->ValidateObject(Target);
			
			UE::AssetValidation::AppendAssetValidationMessages(Context, AnimSequenceAsset, EMessageSeverity::Error, ValidationResult.Errors);
			UE::AssetValidation::AppendAssetValidationMessages(Context, AnimSequenceAsset, EMessageSeverity::Warning, ValidationResult.Warnings);

			Result &= ValidationResult.ValidationResult;
		}
	}

	return Result;
}

EDataValidationResult UAssetValidator_AnimationAsset::ValidateCurves(const UAnimationAsset& AnimAsset, const FAssetData& AssetData, FDataValidationContext& Context)
{
	const USkeleton* Skeleton = AnimAsset.GetSkeleton();
	if (!IsValid(Skeleton))
	{
		return EDataValidationResult::Valid;
	}

	const UAnimSequenceBase* AnimSequence = Cast<UAnimSequenceBase>(&AnimAsset);
	if (AnimSequence == nullptr)
	{
		return EDataValidationResult::Valid;
	}
	
	const FAssetData SkeletonAsset{Skeleton};
	FAssetData AnimSequenceAsset{AnimSequence};
	const TArray<FFloatCurve>& Curves = AnimSequence->GetCurveData().FloatCurves;

	EDataValidationResult Result = EDataValidationResult::Valid;
	for (const FFloatCurve& Curve: Curves)
	{
		const FName CurveName = Curve.GetName();
		if (!CurveExists(*Skeleton, CurveName))
		{
			const FText FailReason = FText::Format(LOCTEXT("NoCurveFormat", "No curve [{0}] present on skeleton "), FText::FromName(CurveName));
			const FText FailReasonEnd = LOCTEXT("CurveExistsOnAsset", ", but exists on animation sequence.");
			if (AssetData == AnimSequenceAsset)
			{
				UE::AssetValidation::AddTokenMessage(Context, EMessageSeverity::Error, AssetData,
					FailReason, SkeletonAsset, FailReasonEnd
				);
			}
			else
			{
				UE::AssetValidation::AddTokenMessage(Context, EMessageSeverity::Error, AssetData,
					FString{" "}, AnimSequenceAsset,
					FailReason, SkeletonAsset, FailReasonEnd
				);
			}

			Result &= EDataValidationResult::Invalid;
		}
	}

	return Result;
}

bool UAssetValidator_AnimationAsset::CurveExists(const USkeleton& Skeleton, FName CurveName)
{
	return Skeleton.GetCurveMetaData(CurveName) != nullptr;
}

#undef LOCTEXT_NAMESPACE
