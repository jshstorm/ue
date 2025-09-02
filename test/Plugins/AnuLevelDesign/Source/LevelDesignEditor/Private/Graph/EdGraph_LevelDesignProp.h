// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once
#include "LevelDesign.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph_LevelDesignProp.generated.h"

struct FLevelDesignDataTypes 
{
	static const FName PinType_Entry;
	static const FName PinType_Action;
	static const FName PinType_Input;
	static const FName PinType_Mission;
};

UCLASS()
class UEdGraph_LevelDesignProp : public UEdGraph 
{
	GENERATED_UCLASS_BODY()

public:
	void InitializeGraph(ULevelDesign* DataAsset);
	void RefreshNodeSelection(UEdGraphNode* Node);
};
