// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "CCNodeFactory.h"
#include "SCCStandardNode.h"
#include "Graph/LDNode_Base.h"

TSharedPtr<class SGraphNode> FCCNodeFactory::CreateNode(UEdGraphNode* Node) const
{	
	if (ULDNode_Base* BaseNode = Cast<ULDNode_Base>(Node))
	{
		return SNew(SCCStandardNode, BaseNode);
	}

	return NULL;
}
