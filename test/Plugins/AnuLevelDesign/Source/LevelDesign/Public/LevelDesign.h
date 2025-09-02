// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once
#include "LevelDesignDefine.h"
#include "LevelDesign.generated.h"

class ANULEVELDESIGN_API ULDNode;
class ANULEVELDESIGN_API ULDObjectiveScript;

UCLASS(Blueprintable)
class ANULEVELDESIGN_API ULevelDesign : public UObject 
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UEdGraph* UpdateGraph;

	UPROPERTY()
	ULDNode* RootNode;

	UPROPERTY(VisibleAnywhere)
	TMap<uint32, ULDNode*> Nodes;

	UPROPERTY(VisibleAnywhere)
	TMap<uint32, ULDObjectiveScript*> Scripts;


#if WITH_EDITOR
	inline static ULevelDesign* EditTarget = nullptr;

	static void SetEditTarget(ULevelDesign* inst)
	{
		EditTarget = inst;
	}
#endif
};