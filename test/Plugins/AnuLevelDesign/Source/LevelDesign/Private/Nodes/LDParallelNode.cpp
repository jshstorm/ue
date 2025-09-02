// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "Nodes/LDParallelNode.h"
#include "LDActionScript.h"

bool ULDParallelNode::UpdateActions(uint32 dt)
{
	bool complete = true;

	for (int32 i = 0; i < ActionScripts.Num(); ++i) {

		if (ActionScripts[i]->_state == ELDNodeState::Deactivate) {
			continue;
		}

		if (ActionScripts[i]->_delayTimer > 0) {
			ActionScripts[i]->_delayTimer -= dt;
			complete = false;
			continue;
		}

		if (ActionScripts[i]->_state == ELDNodeState::None) {
			ActionScripts[i]->Start();
		}

		if (ActionScripts[i]->Update(dt) == false) {
			complete = false;
			continue;
		}

		if (ActionScripts[i]->End() == false) {
			complete = false;
		}
	}

	return complete;
}