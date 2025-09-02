// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "LDNode_Sequence.h"
#include "Nodes/LDSequenceNode.h"
#include "LDEditorStyle.h"
#include "EdGraph_LevelDesignProp.h"

#define LOCTEXT_NAMESPACE "LDNode_Sequence"

ULDNode_Sequence::ULDNode_Sequence(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssociatedObject = ObjectInitializer.CreateDefaultSubobject<ULDSequenceNode>(GetOuter(), *(GetName() + "_" + FString::FromInt(GetUniqueID())));
	
	FString name = FString::Format(TEXT("Sequence ({0})"), { AssociatedObject->UniqueId });

	SetTitle(*name);
}

void ULDNode_Sequence::OnChangeAssociatedNode()
{
	BoxBrush = MakeShared<FSlateBoxBrush>(FLDEditorStyle::GetImageName("bg_gray"), FMargin(4.f / 16.f), FSlateColor(AssociatedObject->Color));
}

const FSlateBrush* ULDNode_Sequence::GetNodeIcon() const
{
	return FLDEditorStyle::GetBrush(TEXT("LDEditor.nodeicon.action"));
}

const FSlateBrush* ULDNode_Sequence::GetNodeBackgroundImage() const
{
	return BoxBrush.Get();
}

void ULDNode_Sequence::AllocateDefaultPins()
{
	UEdGraphPin* Inputs = CreatePin(EGPD_Input, FLevelDesignDataTypes::PinType_Action, TEXT("In"));
	UEdGraphPin* Outputs = CreatePin(EGPD_Output, FLevelDesignDataTypes::PinType_Action, TEXT("Out"));
}

#undef LOCTEXT_NAMESPACE