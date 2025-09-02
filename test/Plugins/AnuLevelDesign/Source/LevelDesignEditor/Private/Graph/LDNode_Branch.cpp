// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "LDNode_Branch.h"
#include "Nodes/LDBranchNode.h"
#include "LDEditorStyle.h"
#include "EdGraph_LevelDesignProp.h"

#define LOCTEXT_NAMESPACE "LDNode_Branch"

ULDNode_Branch::ULDNode_Branch(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssociatedObject = ObjectInitializer.CreateDefaultSubobject<ULDBranchNode>(GetOuter(), *(GetName() + "_" + FString::FromInt(GetUniqueID())));

	SetTitle(*FString::Format(TEXT("Branch ({0})"), { AssociatedObject->UniqueId }));
}

const FSlateBrush* ULDNode_Branch::GetNodeIcon() const
{
	return FLDEditorStyle::GetBrush(TEXT("LDEditor.nodeicon.action"));
}

const FSlateBrush* ULDNode_Branch::GetNodeBackgroundImage() const
{
	return FLDEditorStyle::GetBrush("LDEditor.background_purple");
}

void ULDNode_Branch::AllocateDefaultPins()
{
	UEdGraphPin* Inputs = CreatePin(EGPD_Input, FLevelDesignDataTypes::PinType_Action, TEXT("In"));
	UEdGraphPin* Outputs = CreatePin(EGPD_Output, FLevelDesignDataTypes::PinType_Action, TEXT("Out"));
}

#undef LOCTEXT_NAMESPACE