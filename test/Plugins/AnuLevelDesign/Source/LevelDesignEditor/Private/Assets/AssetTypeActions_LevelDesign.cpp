// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "AssetTypeActions_LevelDesign.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"
#include "LevelDesign.h"
#include "LevelDesignEditor.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions2"
#define AssetColor FColor(31, 110, 64);

FText FAssetTypeActions_LevelDesign::GetName() const
{
	return LOCTEXT("FAssetTypeActions_LevelDesign", "AnuLevelDesign");
}

FColor FAssetTypeActions_LevelDesign::GetTypeColor() const
{
	return AssetColor;
}

UClass* FAssetTypeActions_LevelDesign::GetSupportedClass() const
{
	return ULevelDesign::StaticClass();
}

void FAssetTypeActions_LevelDesign::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;
	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		ULevelDesign* PropData = Cast<ULevelDesign>(*ObjIt);
		if (PropData) {
			TSharedRef<FLevelDesignEditor> NewLevelDesignEditor(new FLevelDesignEditor());
			NewLevelDesignEditor->InitLevelDesignEditor(Mode, EditWithinLevelEditor, PropData);
		}
	}
}

#undef LOCTEXT_NAMESPACE
