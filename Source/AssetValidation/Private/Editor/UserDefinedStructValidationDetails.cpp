#include "UserDefinedStructValidationDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "IDetailChildrenBuilder.h"
#include "Engine/UserDefinedStruct.h"

void FUserDefinedStructValidationDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	const TArray<TWeakObjectPtr<UObject>>& Objects = DetailLayout.GetSelectedObjects();
	check(Objects.Num() > 0);

	if (Objects.Num() == 1)
	{
		UserDefinedStruct = CastChecked<UUserDefinedStruct>(Objects[0].Get());

		IDetailCategoryBuilder& StructureCategory = DetailLayout.EditCategory("Validation", NSLOCTEXT("AssetValidation", "ValidationLabel", "Validation"), ECategoryPriority::Uncommon);
		// Layout = MakeShared<FStructValidationInfoLayout>(UserDefinedStruct);
		
		// StructureCategory.AddCustomBuilder(Layout.ToSharedRef());
	}
}
