// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

class ASSETVALIDATION_API AssetValidationStatics
{
public:

#if 0
	/** */
	static bool ContainsCurve(const USkeleton* Skeleton, FName CurveName);
#endif

	static bool CanValidateProperty(FProperty* Property);
	
};
