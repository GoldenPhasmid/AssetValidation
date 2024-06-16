#include "PropertyValidators/StructValidator.h"

UStructValidator::UStructValidator()
{
	Descriptor = FStructProperty::StaticClass();
}

UScriptStruct* UStructValidator::GetNativeScriptStruct(FName StructName)
{
	static UPackage* CoreUObjectPkg = FindObjectChecked<UPackage>(nullptr, TEXT("/Script/CoreUObject"));
	return (UScriptStruct*)StaticFindObjectFastInternal(UScriptStruct::StaticClass(), CoreUObjectPkg, StructName, false, RF_NoFlags, EInternalObjectFlags::None);
}

