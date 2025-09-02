// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "LDNode_MissionFailure.h"
#include "LDEditorStyle.h"
#include "Nodes/LDFinalizeNode.h"

ULDNode_MissionFailure::ULDNode_MissionFailure(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) 
{
	SetTitle("Failure");
}

const FSlateBrush* ULDNode_MissionFailure::GetNodeBackgroundImage() const
{
	return FLDEditorStyle::GetBrush("LDEditor.background_red");
}

const FSlateBrush* ULDNode_MissionFailure::GetNodeIcon() const
{
	return FLDEditorStyle::GetBrush(TEXT("LDEditor.nodeicon.failure"));
}
