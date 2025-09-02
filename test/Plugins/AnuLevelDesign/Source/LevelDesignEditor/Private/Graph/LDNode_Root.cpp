// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "LDNode_Root.h"
#include "Nodes/LDRootNode.h"
#include "LevelDesign.h"
#include "EdGraph_LevelDesignProp.h"
#include "LDEditorStyle.h"

ULDNode_Root::ULDNode_Root(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<ULDRootNode>(*(GetName() + "_" + FString::FromInt(GetUniqueID()))))
{
	AssociatedObject = ObjectInitializer.CreateDefaultSubobject<ULDRootNode>(GetOuter(), *(GetName() + "_" + FString::FromInt(GetUniqueID())));

	if (ULevelDesign* asset = Cast<ULevelDesign>(GetOuter())) {
		LevelDesignAsset = asset;
	}

	bUserDefined = false;
	bShowIndexOrder = false;
	SetTitle("Start");
}

const FSlateBrush* ULDNode_Root::GetNodeIcon() const
{
	return FLDEditorStyle::GetBrush(TEXT("LDEditor.nodeicon.root"));
}

void ULDNode_Root::SetupDataAsset(class ULevelDesign* asset)
{
	if (AssociatedObject) {
		asset->RootNode = Cast<ULDNode>(AssociatedObject);
	}
}

void ULDNode_Root::AllocateDefaultPins()
{
	CreatePin(EGPD_Output, FLevelDesignDataTypes::PinType_Entry, TEXT("Out"));
}