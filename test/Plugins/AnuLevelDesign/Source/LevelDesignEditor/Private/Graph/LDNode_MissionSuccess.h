// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once
#include "LDNode_Mission.h"
#include "LDNode_MissionSuccess.generated.h"

UCLASS()
class ANULEVELDESIGNEDITOR_API ULDNode_MissionSuccess : public ULDNode_Mission 
{
	GENERATED_UCLASS_BODY()

public:
	virtual const FSlateBrush* GetNodeBackgroundImage() const override;
	virtual const FSlateBrush* GetNodeIcon() const override;
};