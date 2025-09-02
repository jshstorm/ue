// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "LDNode_SelectChild.h"
#include "Nodes/LDSelectChildNode.h"
#include "LDEditorStyle.h"
#include "EdGraph_LevelDesignProp.h"

#define LOCTEXT_NAMESPACE "LDNode_SelectChild"

ULDNode_SelectChild::ULDNode_SelectChild(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssociatedObject = ObjectInitializer.CreateDefaultSubobject<ULDSelectChildNode>(GetOuter(), *(GetName() + "_" + FString::FromInt(GetUniqueID())));

	SetTitle(*FString::Format(TEXT("Random ({0})"), { AssociatedObject->UniqueId }));
}

const FSlateBrush* ULDNode_SelectChild::GetNodeIcon() const
{
	return FLDEditorStyle::GetBrush(TEXT("LDEditor.nodeicon.action"));
}

const FSlateBrush* ULDNode_SelectChild::GetNodeBackgroundImage() const
{
	return FLDEditorStyle::GetBrush("LDEditor.background_purple");
}

void ULDNode_SelectChild::AllocateDefaultPins()
{
	UEdGraphPin* Inputs = CreatePin(EGPD_Input, FLevelDesignDataTypes::PinType_Action, TEXT("In"));
	UEdGraphPin* Outputs = CreatePin(EGPD_Output, FLevelDesignDataTypes::PinType_Action, TEXT("Out"));
}

#undef LOCTEXT_NAMESPACE