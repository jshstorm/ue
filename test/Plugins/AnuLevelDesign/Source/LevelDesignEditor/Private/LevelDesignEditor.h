// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once

#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/AssetEditorManager.h"
#include "Toolkits/IToolkit.h"
#include "Misc/NotifyHook.h"
#include "EditorUndoClient.h"
#include "LevelDesign.h"
#include "IDetailsView.h"
#include "Tickable.h"
#include "AssetThumbnail.h"
#include "GraphEditor.h"

class FLevelDesignEditorThumbnailPool : public FAssetThumbnailPool {
public:
	FLevelDesignEditorThumbnailPool(int NumObjectsInPool) : FAssetThumbnailPool(NumObjectsInPool) {}

	static TSharedPtr<FLevelDesignEditorThumbnailPool> Get() { return Instance; }
	static void Create() {
		Instance = MakeShared<FLevelDesignEditorThumbnailPool>(512);
	}
private:
	static TSharedPtr<FLevelDesignEditorThumbnailPool> Instance;
};

class SGraphEditor_LevelDesign : public SGraphEditor {
public:
	// SWidget implementation
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	// End SWidget implementation
};

/*-----------------------------------------------------------------------------
FLevelDesignEditor
-----------------------------------------------------------------------------*/
class FLevelDesignEditor : public FAssetEditorToolkit, public FNotifyHook, public FTickableGameObject
{
public:
	~FLevelDesignEditor();
	// IToolkit interface
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	// End of IToolkit interface

	// FAssetEditorToolkit
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FText GetToolkitName() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	// End of FAssetEditorToolkit

	// FTickableGameObject Interface
	virtual bool IsTickableInEditor() const override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
	// End of FTickableGameObject Interface

	void InitLevelDesignEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, ULevelDesign* PropData);
	ULevelDesign* GetPropBeingEdited() const { return PropBeingEdited; }

	FORCEINLINE TSharedPtr<SGraphEditor> GetGraphEditor() const { return GraphEditor; }

	void ShowObjectDetails(UObject* ObjectProperties);

protected:
	TSharedRef<class SGraphEditor> CreateGraphEditorWidget(UEdGraph* InGraph);

	/** Select every node in the graph */
	void SelectAllNodes();
	/** Whether we can select every node */
	bool CanSelectAllNodes() const;

	/** Deletes all the selected nodes */
	void DeleteSelectedNodes();

	bool CanDeleteNode(class UEdGraphNode* Node);

	/** Delete an array of Material Graph Nodes and their corresponding expressions/comments */
	void DeleteNodes(const TArray<class UEdGraphNode*>& NodesToDelete);

	/** Delete only the currently selected nodes that can be duplicated */
	void DeleteSelectedDuplicatableNodes();

	/** Whether we are able to delete the currently selected nodes */
	bool CanDeleteNodes() const;

	/** Copy the currently selected nodes */
	void CopySelectedNodes();
	/** Whether we are able to copy the currently selected nodes */
	bool CanCopyNodes() const;

	/** Paste the contents of the clipboard */
	void PasteNodes();
	virtual bool CanPasteNodes() const;
	virtual void PasteNodesHere(const FVector2D& Location);

	/** Cut the currently selected nodes */
	void CutSelectedNodes();
	/** Whether we are able to cut the currently selected nodes */
	bool CanCutNodes() const;

	/** Duplicate the currently selected nodes */
	void DuplicateNodes();
	/** Whether we are able to duplicate the currently selected nodes */
	bool CanDuplicateNodes() const;
	
	void OnGraphChanged(const FEdGraphEditAction& Action);
	void HandleGraphChanged();

protected:
	/** Called when "Save" is clicked for this asset */
	virtual void SaveAsset_Execute() override;

	/** Called when the selection changes in the GraphEditor */
	void OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection);
	void GetNodeRecursive(ULDNode* parentNode, TSet<ULDNode*>& outNodes, TSet<ULDObjectiveScript*>& outScripts, bool& duplicated);

protected:
	TSharedPtr<SGraphEditor> GraphEditor;
	TSharedPtr<FUICommandList> GraphEditorCommands;
	TSharedPtr<IDetailsView> PropertyEditor;
	ULevelDesign* PropBeingEdited;
	TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Actions(const FSpawnTabArgs& Args);

	/** Palette of Node actions to perform on the graph */
	TSharedPtr<class SCCGraphPalette> ActionPalette;

	/** Handle to the registered OnGraphChanged delegate. */
	FDelegateHandle OnGraphChangedDelegateHandle;

	bool bGraphStateChanged;
	TArray<class ULDNode_Base*> LastSelectedNodes;

	TArray<class UCCNode_Group*> LastSelectedGroupNodes;

};

