// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "LDNode_MissionSuccess.h"
#include "LDEditorStyle.h"
#include "Nodes/LDFinalizeNode.h"

ULDNode_MissionSuccess::ULDNode_MissionSuccess(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) 
{
	SetTitle("Success");
}

const FSlateBrush* ULDNode_MissionSuccess::GetNodeIcon() const
{
	return FLDEditorStyle::GetBrush(TEXT("LDEditor.nodeicon.success"));
}

const FSlateBrush* ULDNode_MissionSuccess::GetNodeBackgroundImage() const
{
	return FLDEditorStyle::GetBrush("LDEditor.background_green");
}