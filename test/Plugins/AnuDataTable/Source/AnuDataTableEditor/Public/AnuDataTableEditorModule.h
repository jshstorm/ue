// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Toolkits/IToolkitHost.h"
#include "Modules/ModuleInterface.h"
#include "Toolkits/AssetEditorToolkit.h"

#include "IAssetTools.h"
#include "IAssetTypeActions.h"

class IDataTableEditor;
class UDataTable;

class FAnuDataTableEditorModule : public IModuleInterface
{
public:
	// IModuleInterface implementation
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// Creates an instance of table editor object.  Only virtual so that it can be called across the DLL boundary.
	virtual TSharedRef<IDataTableEditor> CreateDataTableEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, UDataTable* Table);

	static const FName AnuDataTableEditorAppIdentifier;

private:
	// All created asset type actions.  Cached here so that we can unregister them during shutdown.
	TArray<TSharedPtr<IAssetTypeActions>> CreatedAssetTypeActions;

	void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
	{
		AssetTools.RegisterAssetTypeActions(Action);
		CreatedAssetTypeActions.Add(Action);
	}

	TSharedRef<IDataTableEditor> CreateResourceModelDataTableEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UDataTable* Table);
	TSharedRef<IDataTableEditor> CreateResourceMeshPartialDataTableEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UDataTable* Table);
	TSharedRef<IDataTableEditor> CreateSkillTimelineDataTableEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UDataTable* Table);
	TSharedRef<IDataTableEditor> CreateSnapshotDataTableEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UDataTable* Table);
};