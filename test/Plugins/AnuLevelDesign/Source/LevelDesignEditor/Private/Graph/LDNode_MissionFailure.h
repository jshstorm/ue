// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once
#include "LDNode_Mission.h"
#include "LDNode_MissionFailure.generated.h"

UCLASS()
class ANULEVELDESIGNEDITOR_API ULDNode_MissionFailure : public ULDNode_Mission 
{
	GENERATED_UCLASS_BODY()

public:
	virtual const FSlateBrush* GetNodeBackgroundImage() const override;
	virtual const FSlateBrush* GetNodeIcon() const override;
};