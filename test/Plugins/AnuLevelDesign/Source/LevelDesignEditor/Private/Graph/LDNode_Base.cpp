// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "LDNode_Base.h"
#include "LDEditorStyle.h"
#include "LDNode.h"
#include "LDNode_Root.h"
#include "LDActionScript.h"
#include "LDConditionScript.h"
#include "EdGraph_LevelDesignProp.h"

#define LOCTEXT_NAMESPACE "LDNode_Base"

void ULDNode_Base::PostEditChangeProperty(struct FPropertyChangedEvent& e)
{
	Super::PostEditChangeProperty(e);

	TArray<FName> RefreshNames = {"NodeName", "AssociatedObject"};
	if (e.Property &&  RefreshNames.Contains(e.Property->GetFName())) {
		GetGraph()->NotifyGraphChanged();
	}
}

FText ULDNode_Base::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromName(AssociatedObject->Title);
}

const FSlateBrush* ULDNode_Base::GetNodeIcon() const
{
	return FEditorStyle::GetBrush(TEXT("Graph.StateNode.Icon"));
}

FSlateColor ULDNode_Base::GetNodeBackgroundColor() const
{
	return FLinearColor(0.08f, 0.08f, 0.08f);
}

FLinearColor ULDNode_Base::GetNodeTitleColor() const
{
	return FLinearColor(0.7f, 0.7f, 0.7f);
}

void ULDNode_Base::NodeConnectionListChanged()
{
	UEdGraphNode::NodeConnectionListChanged();
	GetGraph()->NotifyGraphChanged();
}

void ULDNode_Base::DestroyAllChildNodes()
{
	TArray<ULDNode_Base*> NodesToRemove;
	for (auto Link : GetOutputPin()->LinkedTo)
	{
		ULDNode_Base* OwningNode = Cast<ULDNode_Base>(Link->GetOwningNode());
		NodesToRemove.Add(OwningNode);
	}

	for (int32 Index = 0; Index < NodesToRemove.Num(); Index++)
	{
		NodesToRemove[Index]->DestroyNode();
	}
}

ULDNode_Base* ULDNode_Base::GetParentNode()
{
	UEdGraphPin* MyInputPin = GetInputPin();
	UEdGraphPin* MyParentOutputPin = nullptr;
	if (MyInputPin != nullptr && MyInputPin->LinkedTo.Num() > 0)
	{
		MyParentOutputPin = MyInputPin->LinkedTo[0];
		if (MyParentOutputPin != nullptr)
		{
			if (MyParentOutputPin->GetOwningNode() != nullptr)
			{
				return CastChecked<ULDNode_Base>(MyParentOutputPin->GetOwningNode());
			}
		}
	}

	return nullptr;
}

TArray<ULDNode_Base*> ULDNode_Base::GetParentNodes()
{
	UEdGraphPin* MyInputPin = GetInputPin();
	TArray<ULDNode_Base*> ParentNodes = TArray<ULDNode_Base*>();

	if (MyInputPin != nullptr && MyInputPin->LinkedTo.Num() > 0)
	{
		for (UEdGraphPin* MyParentOutputPin : MyInputPin->LinkedTo)
		{
			if (MyParentOutputPin->GetOwningNode() != nullptr)
			{
				ParentNodes.AddUnique(CastChecked<ULDNode_Base>(MyParentOutputPin->GetOwningNode()));
			}
		}
	}

	return ParentNodes;
}

void ULDNode_Base::PrepareForCopying()
{
	AssociatedObject->Rename(NULL, this, REN_NonTransactional);

	Super::PrepareForCopying();
}

void ULDNode_Base::PostCopying()
{
	AssociatedObject->Rename(NULL, FindRootNode()->LevelDesignAsset, REN_NonTransactional);
}

void ULDNode_Base::PinConnectionListChanged(UEdGraphPin* Pin)
{
	if (GetInputPin() == Pin)
	{
		AssociatedObject->RemoveFromParentNodes();
		for (ULDNode_Base* ParentNode : GetParentNodes())
		{
			ParentNode->AssociatedObject->AddChildNode(AssociatedObject);
		}		
	}
}

void ULDNode_Base::SetTitle(FName NewTitle)
{
	if (IsValid(AssociatedObject))
	{
		AssociatedObject->Title = NewTitle;
	}
}

const FSlateBrush* ULDNode_Base::GetNodeBackgroundImage() const
{
	return FLDEditorStyle::GetBrush("LDEditor.background_blue");
}

TArray<ULDNode_Base*> ULDNode_Base::GetNodeChildren()
{
	TArray<ULDNode_Base*> Children;
	if (UEdGraphPin* Pin = GetOutputPin())
	{
		for (UEdGraphPin* ChildPin : Pin->LinkedTo)
		{
			if (ULDNode_Base* ChildNode = Cast<ULDNode_Base>(ChildPin->GetOwningNode()))
			{
				Children.Add(ChildNode);
			}
		}
	}

	return Children;
}

bool ULDNode_Base::HasAnyParentNodes()
{
	if (UEdGraphPin* Pin = GetInputPin())
	{
		return (Pin->LinkedTo.Num() > 0);
	}
	return false;
}

bool ULDNode_Base::HasAnyChildNodes()
{
	if (UEdGraphPin* Pin = GetOutputPin())
	{
		return (Pin->LinkedTo.Num() > 0);
	}

	return false;
}

void ULDNode_Base::CreateModelChilds()
{
	for (ULDNode_Base* Child : GetNodeChildren())
	{
		AssociatedObject->AddChildNode(Child->AssociatedObject);
	}
}

void ULDNode_Base::AutowireNewNode(UEdGraphPin* FromPin)
{
	if (!FromPin) {
		return;
	}

	UEdGraphPin* InputPin = GetInputPin();

	// Make sure we have no loops with this connection
	const UEdGraphSchema* Schema = GetGraph()->GetSchema();
	const FPinConnectionResponse ConnectionValid = Schema->CanCreateConnection(FromPin, InputPin);
	if (ConnectionValid.Response == CONNECT_RESPONSE_MAKE)
	{
		FromPin->MakeLinkTo(InputPin);

		FromPin->GetOwningNode()->PinConnectionListChanged(FromPin);
		PinConnectionListChanged(InputPin);
	}
}

void ULDNode_Base::DestroyNode()
{
	BreakAllNodeLinks();
	AssociatedObject->RemoveFromParentNodes();
	Super::DestroyNode();
}

void ULDNode_Base::PostPasteNode()
{
	// Set parent node to nullptr and removing children because created objects are new (invalid)
	AssociatedObject->RemoveFromParentNodes();
	AssociatedObject->ClearChildNodes();
}

void ULDNode_Base::PostPasteNodeFinal()
{
	// Get new unique name from appending new unique id
	FString Left = "";
	AssociatedObject->GetName().Split("_", &Left, nullptr, ESearchCase::IgnoreCase, ESearchDir::FromEnd);
	FString NewName = Left + "_" + FString::FromInt(GetUniqueID());
	AssociatedObject->Rename(*NewName, FindRootNode()->LevelDesignAsset, REN_NonTransactional);
	
	FString oldTitle = AssociatedObject->Title.ToString();
	int32 indexAt = oldTitle.Find("(");
	oldTitle.RemoveAt(indexAt, oldTitle.Len() - indexAt);
	FString newTitle = FString::Format(TEXT("{0} ({1})"), { oldTitle, AssociatedObject->UniqueId });
	AssociatedObject->Title = FName(newTitle);

	if (HasAnyChildNodes())
	{
		CreateModelChilds();
	}

	for (auto script : AssociatedObject->ActionScripts) {
		script->GenerateUniqueId();
	}

	for (auto script : AssociatedObject->ConditionScripts) {
		script->GenerateUniqueId();
	}
}

class ULDNode_Root* ULDNode_Base::FindRootNode()
{
	if (GetOuter()->IsA(UEdGraph::StaticClass()))
	{
		TArray<ULDNode_Root*> RootNodes;
		GetGraph()->GetNodesOfClass(RootNodes);
		if (RootNodes.Num() > 0)
		{
			return RootNodes[0];
		}
	}
	return nullptr;
}

#undef LOCTEXT_NAMESPACE
