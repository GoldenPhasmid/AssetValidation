// Copyright Epic Games, Inc. All Rights Reserved.

#include "AssetValidationStatics.h"

#if 0
bool AssetValidationStatics::ContainsCurve(const USkeleton* Skeleton, FName CurveName)
{
	check(IsValid(Skeleton));

	FSmartName SmartName;
	auto FindSmartName = [&](const FName& ContainerName)
	{
		const FSmartNameMapping* SmartNameContainer = Skeleton->GetSmartNameContainer(ContainerName);
		return SmartNameContainer->FindSmartName(CurveName, SmartName);
	};

	if (FindSmartName(USkeleton::AnimCurveMappingName) || FindSmartName(USkeleton::AnimTrackCurveMappingName))
	{
		return true;
	}

	return false;
}
#endif

bool AssetValidationStatics::CanValidateProperty(FProperty* Property)
{
	return Property != nullptr;
}
