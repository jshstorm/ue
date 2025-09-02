// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "LDNode_Mission.h"
#include "Nodes/LDFinalizeNode.h"
#include "EdGraph_LevelDesignProp.h"

ULDNode_Mission::ULDNode_Mission(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AssociatedObject = ObjectInitializer.CreateDefaultSubobject<ULDFinalizeNode>(GetOuter(), *(GetName() + "_" + FString::FromInt(GetUniqueID())));
	bShowIndexOrder = true;
}

void ULDNode_Mission::AllocateDefaultPins()
{
	UEdGraphPin* Inputs = CreatePin(EGPD_Input, FLevelDesignDataTypes::PinType_Mission, TEXT("In"));
}