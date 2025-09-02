// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "EdGraphSchema_LevelDesignProp.h"
#include "EdGraph_LevelDesignProp.h"
#include "EdGraph/EdGraphPin.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"

#include "LDConnectionDrawingPolicy.h"
#include "LDNode_Root.h"
#include "LDNode_Base.h"

#define SNAP_GRID (16)

#define LOCTEXT_NAMESPACE "LevelDesignSchema"

namespace
{
	// Maximum distance a drag can be off a node edge to require 'push off' from node
	const int32 NodeDistance = 60;
}

UEdGraphNode* FLevelDesignSchemaAction::PerformAction(class UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode/* = true*/)
{
	UEdGraphNode* ResultNode = NULL;

	// If there is a template, we actually use it
	if (NodeTemplate != NULL)
	{
		NodeTemplate->SetFlags(RF_Transactional);

		// set outer to be the graph so it doesn't go away
		NodeTemplate->Rename(NULL, ParentGraph, REN_NonTransactional);
		ParentGraph->AddNode(NodeTemplate, true, bSelectNewNode);

		NodeTemplate->CreateNewGuid();
		NodeTemplate->PostPlacedNewNode();
		NodeTemplate->AllocateDefaultPins();
		NodeTemplate->AutowireNewNode(FromPin);

		// For input pins, new node will generally overlap node being dragged off
		// Work out if we want to visually push away from connected node
		int32 XLocation = Location.X;
		if (FromPin && FromPin->Direction == EGPD_Input)
		{
			UEdGraphNode* PinNode = FromPin->GetOwningNode();
			const float XDelta = FMath::Abs(PinNode->NodePosX - Location.X);

			if (XDelta < NodeDistance)
			{
				// Set location to edge of current node minus the max move distance
				// to force node to push off from connect node enough to give selection handle
				XLocation = PinNode->NodePosX - NodeDistance;
			}
		}

		NodeTemplate->NodePosX = XLocation;
		NodeTemplate->NodePosY = Location.Y;
		NodeTemplate->SnapToGrid(SNAP_GRID);

		ResultNode = NodeTemplate;
	}

	return ResultNode;
}

UEdGraphNode* FLevelDesignSchemaAction::PerformAction(class UEdGraph* ParentGraph, TArray<UEdGraphPin*>& FromPins, const FVector2D Location, bool bSelectNewNode/* = true*/)
{
	UEdGraphNode* ResultNode = NULL;

	if (FromPins.Num() > 0)
	{
		ResultNode = PerformAction(ParentGraph, FromPins[0], Location, bSelectNewNode);

		// Try autowiring the rest of the pins
		for (int32 Index = 1; Index < FromPins.Num(); ++Index)
		{
			ResultNode->AutowireNewNode(FromPins[Index]);
		}
	}
	else
	{
		ResultNode = PerformAction(ParentGraph, NULL, Location, bSelectNewNode);
	}

	return ResultNode;
}

void FLevelDesignSchemaAction::AddReferencedObjects(FReferenceCollector& Collector)
{
	FEdGraphSchemaAction::AddReferencedObjects(Collector);

	// These don't get saved to disk, but we want to make sure the objects don't get GC'd while the action array is around
	Collector.AddReferencedObject(NodeTemplate);
}

void UEdLevelDesignSchema::GetActionList(TArray<TSharedPtr<FEdGraphSchemaAction>>& OutActions, const UEdGraph* Graph, ELDActionType ActionType) const
{
	if (ULDNode_Root* MONode_Root = GetRootNode(Graph))
	{
		UObject* Owner = MONode_Root->LevelDesignAsset;

		static TSet<UClass*> IgnoreList {
			ULDNode_Root::StaticClass()
		};
		
		for (TObjectIterator<UClass> it; it; ++it) {
			if (it->IsChildOf(ULDNode_Base::StaticClass()) && it->HasAnyClassFlags(CLASS_Abstract) == false && IgnoreList.Contains(*it) == false) {
				UClass* classPtr = *it;
				FString className = classPtr->GetFName().ToString();
				LevelDesignSchemaUtils::AddActionWithClass(classPtr, className, className, OutActions, Owner);
			}
		}
	}
}

void UEdLevelDesignSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	const UEdGraph* Graph = ContextMenuBuilder.CurrentGraph;
	TArray<TSharedPtr<FEdGraphSchemaAction> > Actions;
	GetActionList(Actions, Graph, ELDActionType::All);
	for (TSharedPtr<FEdGraphSchemaAction> Action : Actions) {
		ContextMenuBuilder.AddAction(Action);
	}
}

void UEdLevelDesignSchema::GetContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const
{
	Super::GetContextMenuActions(Menu, Context);
}

FString Combine(const TArray<FString> Array, FString Separator) {
	FString Result;
	for (const FString& Item : Array) {
		if (Result.Len() > 0) {
			Result += Separator;
		}
		Result += Item;
	}
	return Result;
}

const FPinConnectionResponse UEdLevelDesignSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	// Make sure the input is connecting to an output
	if (A->Direction == B->Direction) {
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Not allowed"));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
}

class FConnectionDrawingPolicy* UEdLevelDesignSchema::CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const
{
	return new FLDConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj);
}

FLinearColor UEdLevelDesignSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	return FColor::Yellow;
}

bool UEdLevelDesignSchema::ShouldHidePinDefaultValue(UEdGraphPin* Pin) const
{
	return false;
}

bool UEdLevelDesignSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	bool ConnectionMade = UEdGraphSchema::TryCreateConnection(A, B);
	if (ConnectionMade) {
		UEdGraphPin* OutputPin = (A->Direction == EEdGraphPinDirection::EGPD_Output) ? A : B;
		ULDNode_Base* OutputNode = Cast<ULDNode_Base>(OutputPin->GetOwningNode());
		if (OutputNode) {
			OutputNode->GetGraph()->NotifyGraphChanged();
		}
	}

	return ConnectionMade;
}

ULDNode_Root* UEdLevelDesignSchema::GetRootNode(const UEdGraph* Graph) const
{
	TArray<ULDNode_Root*> RootNodes;
	Graph->GetNodesOfClass(RootNodes);
	if (RootNodes.Num() > 0)
	{
		return RootNodes[0];
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE