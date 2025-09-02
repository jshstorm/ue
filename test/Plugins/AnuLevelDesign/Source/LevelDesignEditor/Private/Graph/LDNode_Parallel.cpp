// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "LDNode_Parallel.h"
#include "Nodes/LDParallelNode.h"
#include "LDEditorStyle.h"
#include "EdGraph_LevelDesignProp.h"

#define LOCTEXT_NAMESPACE "LDNode_Parallel"

ULDNode_Parallel::ULDNode_Parallel(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssociatedObject = ObjectInitializer.CreateDefaultSubobject<ULDParallelNode>(GetOuter(), *(GetName() + "_" + FString::FromInt(GetUniqueID())));

	SetTitle(*FString::Format(TEXT("Parallel ({0})"), { AssociatedObject->UniqueId }));
}

void ULDNode_Parallel::OnChangeAssociatedNode()
{
	BoxBrush = MakeShared<FSlateBoxBrush>(FLDEditorStyle::GetImageName("bg_gray"), FMargin(4.f / 16.f), FSlateColor(AssociatedObject->Color));
}

const FSlateBrush* ULDNode_Parallel::GetNodeIcon() const
{
	return FLDEditorStyle::GetBrush(TEXT("LDEditor.nodeicon.action"));
}

const FSlateBrush* ULDNode_Parallel::GetNodeBackgroundImage() const
{
	return BoxBrush.Get();
}

void ULDNode_Parallel::AllocateDefaultPins()
{
	UEdGraphPin* Inputs = CreatePin(EGPD_Input, FLevelDesignDataTypes::PinType_Action, TEXT("In"));
	UEdGraphPin* Outputs = CreatePin(EGPD_Output, FLevelDesignDataTypes::PinType_Action, TEXT("Out"));
}

#undef LOCTEXT_NAMESPACE