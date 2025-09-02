// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "EdGraph_LevelDesignProp.h"
#include "EdGraphSchema_LevelDesignProp.h"
#include "GraphEditAction.h"
#include "LDNode_Root.h"
#include "LDNode_MissionFailure.h"
#include "LDNode_MissionSuccess.h"
#include "LDNode_Base.h"

#define SNAP_GRID (16)

#define LOCTEXT_NAMESPACE "LevelDesignPropGraph"

const FName FLevelDesignDataTypes::PinType_Entry = "Entry";
const FName FLevelDesignDataTypes::PinType_Input = "Input";
const FName FLevelDesignDataTypes::PinType_Action = "Action";
const FName FLevelDesignDataTypes::PinType_Mission = "Mission";


UEdGraph_LevelDesignProp::UEdGraph_LevelDesignProp(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Schema = UEdLevelDesignSchema::StaticClass();
}

void UEdGraph_LevelDesignProp::InitializeGraph(ULevelDesign* DataAsset)
{
	// ROOT NODE CREATION
	ULDNode_Root* RootNode = NewObject<ULDNode_Root>(DataAsset);
	RootNode->bUserDefined = false;
	RootNode->Rename(NULL, this, REN_NonTransactional);
	this->AddNode(RootNode, true, false);

	RootNode->CreateNewGuid();
	RootNode->PostPlacedNewNode();
	RootNode->AllocateDefaultPins();

	RootNode->NodePosX = 0;
	RootNode->NodePosY = 0;
	RootNode->SnapToGrid(SNAP_GRID);
	RootNode->SetupDataAsset(DataAsset);

}

void UEdGraph_LevelDesignProp::RefreshNodeSelection(UEdGraphNode* Node)
{
	TSet<const UEdGraphNode*> NodesToFocus;
	//NodesToFocus.Add(Node);
	
	FEdGraphEditAction SelectionAction;
	SelectionAction.Action = GRAPHACTION_SelectNode;
	SelectionAction.Graph = this;
	NotifyGraphChanged(SelectionAction);
	SelectionAction.Nodes = NodesToFocus;
	NotifyGraphChanged(SelectionAction);
}

#undef LOCTEXT_NAMESPACE