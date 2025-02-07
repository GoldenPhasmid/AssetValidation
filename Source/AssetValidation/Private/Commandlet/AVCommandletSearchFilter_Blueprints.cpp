#include "Commandlet/AVCommandletSearchFilter_Blueprints.h"

UAVCommandletSearchFilter_Blueprints::UAVCommandletSearchFilter_Blueprints()
{
	bWidgets = true;
}

void UAVCommandletSearchFilter_Blueprints::AddClassPaths(TArray<FTopLevelAssetPath>& ClassPaths) const
{
	Super::AddClassPaths(ClassPaths);

	ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	ClassPaths.Add(UAnimBlueprint::StaticClass()->GetClassPathName());
	AddDerivedClasses(ClassPaths, UBlueprint::StaticClass());
}
