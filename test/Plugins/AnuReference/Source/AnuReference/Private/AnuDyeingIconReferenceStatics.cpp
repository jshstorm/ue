// Copyright 2017 CLOVERGAMES Co., Ltd. All right reserved.

#include "AnuDyeingIconReferenceStatics.h"
#include "Path.h"
#include "LogAnuReference.h"

bool UAnuDyeingIconReferenceStatics::IsDyeingIconReference(const FSoftObjectPath& softObjectPath)
{
	const FPath AssetPath = softObjectPath.GetAssetPathString();

	// Textures for dyeing icon must contained in "Diffuse" directory.
	return AssetPath.GetParent().GetFilename() == TEXT("Diffuse");
}

FSoftObjectPath UAnuDyeingIconReferenceStatics::GetDyeingMaskIconReference(const FSoftObjectPath& softObjectPath)
{
	constexpr TCHAR MaskFilenameAppend[] = TEXT("_masking");
	const FPath AssetPath = softObjectPath.GetAssetPathString();

	// The asset contains in Diffuse directory, that is dyeing icon.
	const FPath MaskPath = AssetPath.GetParent().GetRelativePath(TEXT("../Mask"));

	FString maskedAssetFilename = (AssetPath.GetFilename() + MaskFilenameAppend);
	maskedAssetFilename += TEXT(".") + maskedAssetFilename;

	const FSoftObjectPath MaskAssetPath = *(MaskPath / maskedAssetFilename);
	return MaskAssetPath;
}