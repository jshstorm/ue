// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "LevelDesignEditorModule.h"
#include "LDEditorStyle.h"
#include "LevelDesignEditor.h"
#include "IAssetTools.h"
#include "Modules/ModuleManager.h"
#include "EdGraphUtilities.h"	
#include "EditorModeRegistry.h"
#include "Assets/AssetTypeActions_LevelDesign.h"
#include "Widgets/CCNodeFactory.h"
#include "AssetTypeCategories.h"

#define LOCTEXT_NAMESPACE "FLevelDesignEditorModule"

FLevelDesignEditorModule::FLevelDesignEditorModule() : LevelDesignCategoryBit(EAssetTypeCategories::UI)
{
}

void FLevelDesignEditorModule::StartupModule()
{
	FLDEditorStyle::Initialize();
	FLDEditorStyle::ReloadTextures();

#ifdef WITH_EDITOR_ONLY
	// Thumbnails
	FLevelDesignEditorThumbnailPool::Create();
#endif // WITH_EDITOR_ONLY

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	
	// Register LevelDesign asset category
	LevelDesignCategoryBit = AssetTools.RegisterAdvancedAssetCategory("Anu", FText::FromString("LevelDesign"));

	// Register asset types
	RegisterAssetTypeAction(AssetTools, MakeShared<FAssetTypeActions_LevelDesign>(LevelDesignCategoryBit));

	// Register custom graph nodes
	GraphPanelNodeFactory = MakeShared<FCCNodeFactory>();
	FEdGraphUtilities::RegisterVisualNodeFactory(GraphPanelNodeFactory);
}

void FLevelDesignEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (int32 Index = 0; Index < CreatedAssetTypeActions.Num(); ++Index)
		{
			AssetTools.UnregisterAssetTypeActions(CreatedAssetTypeActions[Index].ToSharedRef());
		}
	}
	CreatedAssetTypeActions.Empty();
}

void FLevelDesignEditorModule::RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
{
	AssetTools.RegisterAssetTypeActions(Action);
	CreatedAssetTypeActions.Add(Action);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FLevelDesignEditorModule, AnuLevelDesignEditor)