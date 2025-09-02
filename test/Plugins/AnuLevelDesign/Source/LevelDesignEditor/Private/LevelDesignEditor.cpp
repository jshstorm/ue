// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "LevelDesignEditor.h"
#include "Toolkits/IToolkitHost.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "GraphEditor.h"
#include "GraphEditorActions.h"
#include "EdGraphUtilities.h"
#include "SNodePanel.h"
#include "Widgets/Docking/SDockTab.h"
#include "SSingleObjectDetailsPanel.h"
#include "Framework/Commands/GenericCommands.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Widgets/SCCGraphPalette.h"
#include "EdGraph/EdGraph.h"
#include "Graph/EdGraph_LevelDesignProp.h"
#include "FileHelpers.h"
#include "HAL/PlatformApplicationMisc.h"

#include "Graph/LDNode_Base.h"
#include "LevelDesignEditorModule.h"
#include "LDNode.h"
#include "Nodes/LDSelectChildNode.h"
#include "LDActionScript.h"
#include "LDConditionScript.h"

#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Policies/PrettyJsonPrintPolicy.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"

#define LOCTEXT_NAMESPACE "FLevelDesignEditor" 

const FName LevelDesignEditorAppName = FName(TEXT("LevelDesignEditorApp"));
TSharedPtr<FLevelDesignEditorThumbnailPool> FLevelDesignEditorThumbnailPool::Instance;

struct FLevelDesignEditorTabs
{
	static const FName DetailsID;
	static const FName ActionsID;
	static const FName ViewportID;
};

const FName FLevelDesignEditorTabs::DetailsID(TEXT("Details"));
const FName FLevelDesignEditorTabs::ViewportID(TEXT("Viewport"));
const FName FLevelDesignEditorTabs::ActionsID(TEXT("Actions"));

FName FLevelDesignEditor::GetToolkitFName() const
{
	return FName("LevelDesignEditor");
}

FText FLevelDesignEditor::GetBaseToolkitName() const
{
	return LOCTEXT("LevelDesignEditorAppLabel", "LevelDesign Editor");
}

FText FLevelDesignEditor::GetToolkitName() const
{
	const bool bDirtyState = PropBeingEdited->GetOutermost()->IsDirty();

	FFormatNamedArguments Args;
	Args.Add(TEXT("LevelDesignName"), FText::FromString(PropBeingEdited->GetName()));
	Args.Add(TEXT("DirtyState"), bDirtyState ? FText::FromString(TEXT("*")) : FText::GetEmpty());
	return FText::Format(LOCTEXT("LevelDesignEditorAppLabel", "{LevelDesignName}{DirtyState}"), Args);
}

FString FLevelDesignEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("LevelDesignEditor");
}

FLinearColor FLevelDesignEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

TSharedRef<SDockTab> FLevelDesignEditor::SpawnTab_Details(const FSpawnTabArgs& Args)
{
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	const FDetailsViewArgs DetailsViewArgs(false, false, true, FDetailsViewArgs::HideNameArea, true, this);
	TSharedPtr<IDetailsView> PropertyEditorRef = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	
	PropertyEditor = PropertyEditorRef;

	// Spawn the tab
	return SNew(SDockTab)
	.Label(LOCTEXT("DetailsTab_Title", "Details"))
	[
		PropertyEditorRef.ToSharedRef()
	];
}

TSharedRef<SDockTab> FLevelDesignEditor::SpawnTab_Actions(const FSpawnTabArgs& Args)
{
	ActionPalette = SNew(SCCGraphPalette, SharedThis(this));

	TSharedRef<SDockTab> SpawnedTab = SNew(SDockTab)
	.Icon(FEditorStyle::GetBrush("Kismet.Tabs.Palette"))
	.Label(LOCTEXT("ActionsPaletteTitle", "Action Nodes"))
	[
		SNew(SBox)
		.AddMetaData<FTagMetaData>(FTagMetaData(TEXT("Nodes")))
		[
			ActionPalette.ToSharedRef()
		]
	];

	return SpawnedTab;
}

bool FLevelDesignEditor::IsTickableInEditor() const
{
	return true;
}

void FLevelDesignEditor::Tick(float DeltaTime)
{
	if (bGraphStateChanged) {
		bGraphStateChanged = false;
		HandleGraphChanged();
	}
}

bool FLevelDesignEditor::IsTickable() const
{
	return true;
}

TStatId FLevelDesignEditor::GetStatId() const
{
	return TStatId();
}

TSharedRef<SDockTab> FLevelDesignEditor::SpawnTab_Viewport(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("PropMeshGraph", "Mesh Graph"))
		.TabColorScale(GetTabColorScale())
		[
			GraphEditor.ToSharedRef()
		];
}

TSharedRef<SGraphEditor> FLevelDesignEditor::CreateGraphEditorWidget(UEdGraph* InGraph) {
	// Create the appearance info
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("AppearanceCornerText", "Nodes");

	GraphEditorCommands = MakeShared<FUICommandList>();
	{
		GraphEditorCommands->MapAction(FGenericCommands::Get().SelectAll,
			FExecuteAction::CreateSP(this, &FLevelDesignEditor::SelectAllNodes),
			FCanExecuteAction::CreateSP(this, &FLevelDesignEditor::CanSelectAllNodes)
		);

		GraphEditorCommands->MapAction(FGenericCommands::Get().Delete,
			FExecuteAction::CreateSP(this, &FLevelDesignEditor::DeleteSelectedNodes),
			FCanExecuteAction::CreateSP(this, &FLevelDesignEditor::CanDeleteNodes)
		);

		GraphEditorCommands->MapAction(FGenericCommands::Get().Copy,
			FExecuteAction::CreateSP(this, &FLevelDesignEditor::CopySelectedNodes),
			FCanExecuteAction::CreateSP(this, &FLevelDesignEditor::CanCopyNodes)
		);

		GraphEditorCommands->MapAction(FGenericCommands::Get().Paste,
			FExecuteAction::CreateSP(this, &FLevelDesignEditor::PasteNodes),
			FCanExecuteAction::CreateSP(this, &FLevelDesignEditor::CanPasteNodes)
		);

		GraphEditorCommands->MapAction(FGenericCommands::Get().Cut,
			FExecuteAction::CreateSP(this, &FLevelDesignEditor::CutSelectedNodes),
			FCanExecuteAction::CreateSP(this, &FLevelDesignEditor::CanCutNodes)
		);

		GraphEditorCommands->MapAction(FGenericCommands::Get().Duplicate,
			FExecuteAction::CreateSP(this, &FLevelDesignEditor::DuplicateNodes),
			FCanExecuteAction::CreateSP(this, &FLevelDesignEditor::CanDuplicateNodes)
		);
	}

	SGraphEditor::FGraphEditorEvents InEvents;
	InEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FLevelDesignEditor::OnSelectedNodesChanged);

	// Make title bar
	TSharedRef<SWidget> TitleBarWidget =
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush(TEXT("Graph.TitleBackground")))
		.HAlign(HAlign_Fill)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Center)
			.FillWidth(1.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("UpdateGraphLabel", "LevelDesign Graph"))
				.TextStyle(FEditorStyle::Get(), TEXT("GraphBreadcrumbButtonText"))
			]
		];

	TSharedRef<SGraphEditor> _GraphEditor = SNew(SGraphEditor_LevelDesign)
		.AdditionalCommands(GraphEditorCommands)
		.Appearance(AppearanceInfo)
		//.TitleBar(TitleBarWidget)
		.GraphToEdit(InGraph)
		.GraphEvents(InEvents)
		;

	return _GraphEditor;
}

void FLevelDesignEditor::SelectAllNodes()
{
	GraphEditor->SelectAllNodes();
}

bool FLevelDesignEditor::CanSelectAllNodes() const
{
	return GraphEditor.IsValid();
}

void FLevelDesignEditor::DeleteSelectedNodes()
{
	TArray<UEdGraphNode*> NodesToDelete;
	const FGraphPanelSelectionSet SelectedNodes = GraphEditor->GetSelectedNodes();

	for (FGraphPanelSelectionSet::TConstIterator NodeIt(SelectedNodes); NodeIt; ++NodeIt)
	{
		NodesToDelete.Add(CastChecked<UEdGraphNode>(*NodeIt));
	}

	DeleteNodes(NodesToDelete);
}

bool FLevelDesignEditor::CanDeleteNode(class UEdGraphNode* Node)
{
	bool CanDelete = true;

	if (ULDNode_Base* GroupNode = Cast<ULDNode_Base>(Node)) {
		CanDelete = GroupNode->bUserDefined;
	}

	return CanDelete;
}

void FLevelDesignEditor::DeleteNodes(const TArray<class UEdGraphNode*>& NodesToDelete)
{
	if (NodesToDelete.Num() > 0)
	{

		for (int32 Index = 0; Index < NodesToDelete.Num(); ++Index)
		{
			if (!CanDeleteNode(NodesToDelete[Index])) {
				continue;
			}

			NodesToDelete[Index]->DestroyNode();
		}
	}
}

void FLevelDesignEditor::DeleteSelectedDuplicatableNodes()
{
	// Cache off the old selection
	const FGraphPanelSelectionSet OldSelectedNodes = GraphEditor->GetSelectedNodes();

	// Clear the selection and only select the nodes that can be duplicated
	FGraphPanelSelectionSet RemainingNodes;
	GraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(OldSelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if ((Node != NULL) && Node->CanDuplicateNode())
		{
			GraphEditor->SetNodeSelection(Node, true);
		}
		else
		{
			RemainingNodes.Add(Node);
		}
	}

	// Delete the duplicatable nodes
	DeleteSelectedNodes();

	// Reselect whatever is left from the original selection after the deletion
	GraphEditor->ClearSelectionSet();

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(RemainingNodes); SelectedIter; ++SelectedIter)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
		{
			GraphEditor->SetNodeSelection(Node, true);
		}
	}
}

bool FLevelDesignEditor::CanDeleteNodes() const
{
	return true;
}

void FLevelDesignEditor::CopySelectedNodes()
{
	// Export the selected nodes and place the text on the clipboard

	FGraphPanelSelectionSet SelectedNodes = FGraphPanelSelectionSet();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(GraphEditor->GetSelectedNodes()); SelectedIter; ++SelectedIter)
	{
		if (ULDNode_Base* NodeBase = Cast<ULDNode_Base>(*SelectedIter))
		{
			// Only user defined nodes can be copied
			if (NodeBase->bUserDefined)
			{
				SelectedNodes.Add(*SelectedIter);
			}
		}
	}

	FString ExportedText;
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
		{
			Node->PrepareForCopying();
		}
	}

	FEdGraphUtilities::ExportNodesToText(SelectedNodes, /*out*/ ExportedText);
	FPlatformApplicationMisc::ClipboardCopy(*ExportedText);

	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		if (UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter))
		{
			if (ULDNode_Base* NodeBase = Cast<ULDNode_Base>(Node))
			{
				NodeBase->PostCopying();
			}
		}
	}
}

bool FLevelDesignEditor::CanCopyNodes() const
{
	// If any of the nodes can be duplicated then we should allow copying
	const FGraphPanelSelectionSet SelectedNodes = GraphEditor->GetSelectedNodes();
	for (FGraphPanelSelectionSet::TConstIterator SelectedIter(SelectedNodes); SelectedIter; ++SelectedIter)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(*SelectedIter);
		if ((Node != NULL) && Node->CanDuplicateNode())
		{
			return true;
		}
	}
	return false;
}

void FLevelDesignEditor::PasteNodes()
{
	PasteNodesHere(GraphEditor->GetPasteLocation());
}

void FLevelDesignEditor::PasteNodesHere(const FVector2D& Location)
{
	// Clear the selection set (newly pasted stuff will be selected)
	GraphEditor->ClearSelectionSet();

	// Grab the text to paste from the clipboard.
	FString TextToImport;
	FPlatformApplicationMisc::ClipboardPaste(TextToImport);

	// Import the nodes
	if (!PropBeingEdited) return;
	TSet<UEdGraphNode*> PastedNodes;
	FEdGraphUtilities::ImportNodesFromText(PropBeingEdited->UpdateGraph, TextToImport, /*out*/ PastedNodes);

	//Average position of nodes so we can move them while still maintaining relative distances to each other
	FVector2D AvgNodePosition(0.0f, 0.0f);

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UEdGraphNode* Node = *It;
		AvgNodePosition.X += Node->NodePosX;
		AvgNodePosition.Y += Node->NodePosY;
	}

	if (PastedNodes.Num() > 0)
	{
		float InvNumNodes = 1.0f / float(PastedNodes.Num());
		AvgNodePosition.X *= InvNumNodes;
		AvgNodePosition.Y *= InvNumNodes;
	}

	for (TSet<UEdGraphNode*>::TIterator It(PastedNodes); It; ++It)
	{
		UEdGraphNode* Node = *It;

		// Select the newly pasted stuff
		GraphEditor->SetNodeSelection(Node, true);
		Node->NodePosX = (Node->NodePosX - AvgNodePosition.X) + Location.X;
		Node->NodePosY = (Node->NodePosY - AvgNodePosition.Y) + Location.Y;
		Node->SnapToGrid(SNodePanel::GetSnapGridSize());

		// Give new node a different Guid from the old one
		Node->CreateNewGuid();
		
		if (ULDNode_Base* NodeBase = Cast<ULDNode_Base>(Node))
		{
			if (NodeBase->AssociatedObject) {
				NodeBase->AssociatedObject->GenerateUniqueId();
			}

			NodeBase->PostPasteNodeFinal();
		}
	}

	// Update UI
	GraphEditor->NotifyGraphChanged();
}

bool FLevelDesignEditor::CanPasteNodes() const
{
	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	return FEdGraphUtilities::CanImportNodesFromText(PropBeingEdited->UpdateGraph, ClipboardContent);
}

void FLevelDesignEditor::CutSelectedNodes()
{
	CopySelectedNodes();
	// Cut should only delete nodes that can be duplicated
	DeleteSelectedDuplicatableNodes();
}

bool FLevelDesignEditor::CanCutNodes() const
{
	return CanCopyNodes() && CanDeleteNodes();
}

void FLevelDesignEditor::DuplicateNodes()
{
	// Copy and paste current selection
	CopySelectedNodes();
	PasteNodes();
}

bool FLevelDesignEditor::CanDuplicateNodes() const
{
	return CanCopyNodes();
}

void FLevelDesignEditor::OnGraphChanged(const FEdGraphEditAction& Action)
{
	bGraphStateChanged = true;
}

void FLevelDesignEditor::HandleGraphChanged()
{
	ActionPalette->Refresh();
}

void FLevelDesignEditor::SaveAsset_Execute()
{
	// Refresh
	auto graph = GraphEditor->GetCurrentGraph();
	for (auto one : graph->Nodes) {
		auto ldNode = Cast<ULDNode_Base>(one);
		if (ldNode == nullptr || ldNode->AssociatedObject == nullptr) {
			continue;
		}
		
		ldNode->AssociatedObject->_childNodes.Empty();
		ldNode->AssociatedObject->_parentNodes.Empty();

		if (auto pin = ldNode->GetOutputPin()) {
			for (auto link : pin->LinkedTo) {
				if (auto child = Cast<ULDNode_Base>(link->GetOwningNode())) {
					if (child->AssociatedObject) {
						ldNode->AssociatedObject->_childNodes.Emplace(child->AssociatedObject);
					}
				}
			}
		}

		if (auto pin = ldNode->GetInputPin()) {
			for (auto link : pin->LinkedTo) {
				if (auto parent = Cast<ULDNode_Base>(link->GetOwningNode())) {
					if (parent->AssociatedObject) {
						ldNode->AssociatedObject->_parentNodes.Emplace(parent->AssociatedObject);
					}
				}
			}
		}
	}

	// Validation Check
	bool except = false;
	TSet<ULDNode*> nodes;
	TSet<ULDObjectiveScript*> scripts;
	GetNodeRecursive(PropBeingEdited->RootNode, nodes, scripts, except);
	if (except) {
		return;
	}

	TSet<uint32> nodeuids;
	for (auto node : nodes) {
		nodeuids.Emplace(node->UniqueId);
	}

	TSet<uint32> scriptuids;
	for (auto script : scripts) {
		if (scriptuids.Contains(script->UniqueId)) {
			FString msg = FString::Printf(TEXT("Duplicate script detected\nNode:[%d]\nScript:[%s(%d)]"), script->ParentNode->UniqueId, *script->GetFName().ToString(), script->UniqueId);
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(msg));
			return;
		}
		scriptuids.Emplace(script->UniqueId);
	}

	FString error;
	for (auto node : nodes) {
		if (node->Validate(error) == false) {
			FString msg = FString::Printf(TEXT("Invalid Data\nNode:[%s]\nError:[%s]"), *node->GetName(), *error);
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(msg));
			return;
		}

		if (auto randomNode = Cast<ULDSelectChildNode>(node)) {
			for (auto& pair : randomNode->Probability) {
				if (nodeuids.Contains(pair.Key) == false) {
					FString msg = FString::Printf(TEXT("Invalid Child Node\nNode:[%s]\nScriptUID:[%d]"), *node->GetName(), pair.Key);
					FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(msg));
					return;
				}
			}
		}
	}

	PropBeingEdited->Nodes.Empty();
	for (auto node : nodes) {
		PropBeingEdited->Nodes.Emplace(node->UniqueId, node);
	}

	PropBeingEdited->Scripts.Empty();
	for (auto script : scripts) {
		PropBeingEdited->Scripts.Emplace(script->UniqueId, script);
	}

	// Save uasset
	UPackage* Package = PropBeingEdited->GetOutermost();
	if (Package)
	{
		TArray<UPackage*> PackagesToSave;
		PackagesToSave.Add(Package);
		FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, false, false);
	}

	// Save Json
	FString assetName = PropBeingEdited->GetFName().ToString();
	FString fullPath = FPaths::ProjectDir() + FString::Format(TEXT("Content/ExportedReferences/LevelDesign/{0}.json"), { assetName });
	FString buffer;
	TSharedRef<TJsonWriter<TCHAR>> writer = TJsonWriterFactory<TCHAR>::Create(&buffer);
	writer->WriteObjectStart();
	
	writer->WriteArrayStart("nodes");
	for (auto node : nodes) {
		TSharedPtr<FJsonObject> obj = MakeShared<FJsonObject>();
		node->PushData(obj);
		FJsonSerializer::Serialize(obj.ToSharedRef(), writer, false);
	}
	writer->WriteArrayEnd();

	writer->WriteObjectEnd();
	writer->Close();

	FFileHelper::SaveStringToFile(buffer, *fullPath);
}

void FLevelDesignEditor::GetNodeRecursive(ULDNode* parentNode, TSet<ULDNode*>& outNodes, TSet<ULDObjectiveScript*>& outScripts, bool& duplicated)
{
	if (duplicated) {
		return;
	}

	if (outNodes.Contains(parentNode)) {
		return;
	}

	outNodes.Emplace(parentNode);

	for (auto script : parentNode->ConditionScripts) {
		if (script) {
			script->ParentNode = parentNode;

			if (outScripts.Contains(script)) {
				duplicated = true;
				FString msg = FString::Printf(TEXT("Duplicate script detected\nNode:[%d]\nScript:[%s(%d)]"), parentNode->UniqueId, *script->GetFName().ToString(), script->UniqueId);
				FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(msg));
				return;
			}
			outScripts.Emplace(script);
		}
	}

	for (auto script : parentNode->ActionScripts) {
		if (script) {
			script->ParentNode = parentNode;

			if (outScripts.Contains(script)) {
				duplicated = true;
				FString msg = FString::Printf(TEXT("Duplicate script detected\nNode:[%d]\nScript:[%s(%d)]"), parentNode->UniqueId, *script->GetFName().ToString(), script->UniqueId);
				FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(msg));
				return;
			}
			outScripts.Emplace(script);
		}
	}

	for (auto childNode : parentNode->GetChildNodes()) {
		GetNodeRecursive(childNode, outNodes, outScripts, duplicated);
	}
}

void FLevelDesignEditor::OnSelectedNodesChanged(const TSet<class UObject*>& NewSelection)
{
	for (ULDNode_Base* SelectedNode : LastSelectedNodes)
	{
		SelectedNode->bIsNodeSelected = false;
	}

	TArray<UObject*> SelectedObjects;
	for (UObject* Object : NewSelection) {
		SelectedObjects.Add(Object);

		ULDNode_Base* Node = Cast<ULDNode_Base>(Object);
		if (Node)
		{
			LastSelectedNodes.Add(Node);
			Node->bIsNodeSelected = true;
		}
	}

	if (SelectedObjects.Num() == 0)
	{
		PropertyEditor->SetObject(GetPropBeingEdited());
		return;
	}

	PropertyEditor->SetObjects(SelectedObjects);
}

FLevelDesignEditor::~FLevelDesignEditor()
{
	if (GraphEditor->GetCurrentGraph()) {
		GraphEditor->GetCurrentGraph()->RemoveOnGraphChangedHandler(OnGraphChangedDelegateHandle);
	}
}

void FLevelDesignEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManagerReg)
{
	WorkspaceMenuCategory = TabManagerReg->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_LevelDesignEditor", "LevelDesign Editor"));
	auto WorkspaceMenuCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(TabManagerReg);

	TabManagerReg->RegisterTabSpawner(FLevelDesignEditorTabs::DetailsID, FOnSpawnTab::CreateSP(this, &FLevelDesignEditor::SpawnTab_Details))
		.SetDisplayName(LOCTEXT("DetailsTabLabel", "Details"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));

	TabManagerReg->RegisterTabSpawner(FLevelDesignEditorTabs::ViewportID, FOnSpawnTab::CreateSP(this, &FLevelDesignEditor::SpawnTab_Viewport))
		.SetDisplayName(LOCTEXT("ViewportTab", "Viewport"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Viewports"));

	TabManagerReg->RegisterTabSpawner(FLevelDesignEditorTabs::ActionsID, FOnSpawnTab::CreateSP(this, &FLevelDesignEditor::SpawnTab_Actions))
		.SetDisplayName(LOCTEXT("ActionsTabLabel", "Actions"))
		.SetGroup(WorkspaceMenuCategoryRef)
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tabs.Details"));
}

void FLevelDesignEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManagerReg)
{
	FAssetEditorToolkit::UnregisterTabSpawners(TabManagerReg);

	TabManagerReg->UnregisterTabSpawner(FLevelDesignEditorTabs::ViewportID);
	TabManagerReg->UnregisterTabSpawner(FLevelDesignEditorTabs::ActionsID);
	TabManagerReg->UnregisterTabSpawner(FLevelDesignEditorTabs::DetailsID);
}

void FLevelDesignEditor::InitLevelDesignEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, ULevelDesign* PropData) {

	// Initialize the asset editor and spawn nothing (dummy layout)
	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->CloseOtherEditors(PropData, this);
	PropBeingEdited = PropData;
	PropBeingEdited->SetEditTarget(PropData);

	if (!PropBeingEdited->UpdateGraph) {
		UEdGraph_LevelDesignProp* LevelDesignGraph = NewObject<UEdGraph_LevelDesignProp>(PropBeingEdited, UEdGraph_LevelDesignProp::StaticClass(), NAME_None, RF_Transactional);
		LevelDesignGraph->InitializeGraph(PropData);
		PropBeingEdited->UpdateGraph = LevelDesignGraph;
	}

	GraphEditor = CreateGraphEditorWidget(PropBeingEdited->UpdateGraph);
  
	// Default layout
	const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("LevelDesignPropEditor_Layout_v1")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Vertical)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.1f)
				->SetHideTabWell(true)
				->AddTab(GetToolbarTabId(), ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Horizontal)
				->SetSizeCoefficient(0.8f)
				->Split
				(
					FTabManager::NewStack()
					->SetSizeCoefficient(0.8f)
					->SetHideTabWell(true)
					->AddTab(FLevelDesignEditorTabs::ViewportID, ETabState::OpenedTab)
				)
				->Split
				(
					FTabManager::NewSplitter()
					->SetOrientation(Orient_Vertical)
					->SetSizeCoefficient(0.37f)
					->Split
					(
						FTabManager::NewSplitter()
						->SetOrientation(Orient_Horizontal)
						->SetSizeCoefficient(0.33f)
						->Split
						(
							FTabManager::NewStack()
							->SetSizeCoefficient(0.6f)
							->AddTab(FLevelDesignEditorTabs::DetailsID, ETabState::OpenedTab)
						)
					)
					->Split
					(
						FTabManager::NewStack()
						->SetSizeCoefficient(0.6f)
						->AddTab(FLevelDesignEditorTabs::ActionsID, ETabState::OpenedTab)
					)
				)
			)
		);

	// Initialize the asset editor and spawn nothing (dummy layout)
	InitAssetEditor(Mode, InitToolkitHost, LevelDesignEditorAppName, StandaloneDefaultLayout, /*bCreateDefaultStandaloneMenu=*/ true, /*bCreateDefaultToolbar=*/ true, PropData);

	// Listen for graph changed event
	OnGraphChangedDelegateHandle = GraphEditor->GetCurrentGraph()->AddOnGraphChangedHandler(FOnGraphChanged::FDelegate::CreateRaw(this, &FLevelDesignEditor::OnGraphChanged));
	bGraphStateChanged = true;

	ShowObjectDetails(PropBeingEdited);
}

void FLevelDesignEditor::ShowObjectDetails(UObject* ObjectProperties)
{
	PropertyEditor->SetObject(ObjectProperties);
}

void SGraphEditor_LevelDesign::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SGraphEditor::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

#undef LOCTEXT_NAMESPACE