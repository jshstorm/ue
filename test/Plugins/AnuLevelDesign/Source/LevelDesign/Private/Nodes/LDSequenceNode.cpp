// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "Nodes/LDSequenceNode.h"
#include "LDActionScript.h"

bool ULDSequenceNode::UpdateActions(uint32 dt)
{
	for (uint8 i = _runningIndex; i < ActionScripts.Num(); ++i) {
		if (ActionScripts[i]->_delayTimer > 0) {
			ActionScripts[i]->_delayTimer -= dt;
			return false;
		}

		if (ActionScripts[i]->_state == ELDNodeState::None) {
			ActionScripts[i]->Start();
		}

		if (ActionScripts[i]->Update(dt) == false) {
			return false;
		}

		if (ActionScripts[i]->End() == false) {
			return false;
		}

		++_runningIndex;
	}

	return true;
}

void ULDSequenceNode::Reset()
{
	Super::Reset();

	_runningIndex = 0;
}