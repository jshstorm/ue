// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once
#include "LDNode.h"
#include "UObject/NoExportTypes.h"
#include "LDParallelNode.generated.h"

UCLASS(Blueprintable)
class ANULEVELDESIGN_API ULDParallelNode : public ULDNode
{
	GENERATED_BODY()

protected:
	virtual bool UpdateActions(uint32 dt) final;
};