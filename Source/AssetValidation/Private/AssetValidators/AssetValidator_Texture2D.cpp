#include "AssetValidators/AssetValidator_Texture2D.h"

#include "AssetValidationDefines.h"
#include "AssetValidationModule.h"
#include "AssetValidationStatics.h"

#define LOCTEXT_NAMESPACE "AssetValidation"

UAssetValidator_Texture2D::UAssetValidator_Texture2D()
{
	bIsConfigDisabled = false; // enabled by default

	bCanRunParallelMode = false;
	bRequiresLoadedAsset = false;
	bRequiresTopLevelAsset = true;
	bCanValidateActors = false;
}

bool UAssetValidator_Texture2D::IsEnabled() const
{
	return Super::IsEnabled();
}

bool UAssetValidator_Texture2D::CanValidateAsset_Implementation(const FAssetData& InAssetData, UObject* InObject, FDataValidationContext& InContext) const
{
	if (!Super::CanValidateAsset_Implementation(InAssetData, InObject, InContext))
	{
		return false;
	}

	if (UTexture2D* Texture = Cast<UTexture2D>(InObject))
	{
		return true;
	}
	
	return false;
}

EDataValidationResult UAssetValidator_Texture2D::ValidateLoadedAsset_Implementation(const FAssetData& InAssetData, UObject* InAsset, FDataValidationContext& Context)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(UAssetValidator_Texture2D, AssetValidationChannel);
	
	UTexture2D* Texture = CastChecked<UTexture2D>(InAsset);
	check(Texture);

	EDataValidationResult Result = EDataValidationResult::Valid;
	if (Texture->LODGroup == TextureGroup::TEXTUREGROUP_UI)
	{
		if (Texture->GetSizeX() % 4 != 0 || Texture->GetSizeY() % 4 != 0)
		{
			UE::AssetValidation::AddTokenMessage(Context, EMessageSeverity::Error, InAssetData,
				LOCTEXT("Texture2D_InvalidSize", "Texture size should be divisible by 4 to enable compression or generate mips.")
			);
			Result &= EDataValidationResult::Invalid;
		}
	}

	const int32 NumPixels = Texture->GetSizeX() * Texture->GetSizeY();
	if (Texture->CompressionSettings == TC_EditorIcon && MaxTextureSizeWithoutCompression != 0
		&& NumPixels >= MaxTextureSizeWithoutCompression * MaxTextureSizeWithoutCompression)
	{
		UE::AssetValidation::AddTokenMessage(Context, EMessageSeverity::Error, InAssetData,
			LOCTEXT("Texture2D_EnableCompression", "Enable BC7 compression, texture is too large.")
		);
		Result &= EDataValidationResult::Invalid;
	}
	
	if (Result != EDataValidationResult::Invalid)
	{
		AssetPasses(InAsset);
	}

	return Result;
}

#undef LOCTEXT_NAMESPACE