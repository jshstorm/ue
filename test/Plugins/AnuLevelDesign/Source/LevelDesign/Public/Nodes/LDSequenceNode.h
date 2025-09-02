// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once
#include "LDNode.h"
#include "UObject/NoExportTypes.h"
#include "LDSequenceNode.generated.h"

UCLASS(Blueprintable)
class ANULEVELDESIGN_API ULDSequenceNode : public ULDNode
{
	GENERATED_BODY()

private:
	uint8 _runningIndex = 0;

protected:
	virtual bool UpdateActions(uint32 dt) final;
	virtual void Reset() final;
};