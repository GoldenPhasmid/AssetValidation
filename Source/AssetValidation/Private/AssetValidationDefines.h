#pragma once

/** 5.4 transition helper */
#ifndef WITH_DATA_VALIDATION_UPDATE
#define WITH_DATA_VALIDATION_UPDATE ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 4
#endif

/** asset validation log */
ASSETVALIDATION_API DECLARE_LOG_CATEGORY_EXTERN(LogAssetValidation, Log, All);
/** asset validation trace channel */
UE_TRACE_CHANNEL_EXTERN(AssetValidationChannel);

FORCEINLINE void operator&=(EDataValidationResult& Lhs, EDataValidationResult Rhs)
{
	Lhs = CombineDataValidationResults(Lhs, Rhs);
}