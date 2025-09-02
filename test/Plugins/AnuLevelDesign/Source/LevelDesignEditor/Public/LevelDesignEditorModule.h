// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IAssetTypeActions.h"
#include "EdGraphUtilities.h"
#include "IAssetTools.h"

class FLevelDesignEditorModule : public IModuleInterface
{
public:
	FLevelDesignEditorModule();

	// IModuleInterface implementation
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// Factory for slate nodes
	TSharedPtr<FGraphPanelNodeFactory> GraphPanelNodeFactory;

private:
	// The menu for Mission asset
	EAssetTypeCategories::Type LevelDesignCategoryBit;

	void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action);

	/** All created asset type actions.  Cached here so that we can unregister them during shutdown. */
	TArray< TSharedPtr<IAssetTypeActions> > CreatedAssetTypeActions;
};
