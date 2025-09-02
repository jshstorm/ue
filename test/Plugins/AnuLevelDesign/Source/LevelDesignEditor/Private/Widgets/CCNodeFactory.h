// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once
#include "EdGraphUtilities.h"

class ANULEVELDESIGNEDITOR_API FCCNodeFactory : public FGraphPanelNodeFactory
{
private:
	virtual TSharedPtr<class SGraphNode> CreateNode(UEdGraphNode* Node) const override;
};
