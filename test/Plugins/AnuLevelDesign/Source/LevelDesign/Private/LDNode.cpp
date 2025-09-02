// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "LDNode.h"
#include "LDConditionScript.h"
#include "LDActionScript.h"


ULDNode::ULDNode()
{
	GenerateUniqueId();
}

void ULDNode::GenerateUniqueId()
{
	UniqueId = GetUniqueID();
}

void ULDNode::Build(FLDBlackboard* blackboard)
{
	if (_builded) {
		return;
	}

	_builded = true;

	for (auto script : ConditionScripts) {
		if (script->IsBuilded() == false) {
			script->Build(blackboard);
		}
	}
	for (auto script : ActionScripts) {
		if (script->IsBuilded() == false) {
			script->Build(blackboard);
		}
	}
	for (auto node : _childNodes) {
		node->Build(blackboard);
	}
}

void ULDNode::PopData(FNetPacket* packet)
{
}

void ULDNode::Activate()
{
#if DEBUG_LD
	UE_LOG(LogLD, Verbose, TEXT("[LD] Node Activate : [%d]"), UniqueId);
#endif

	_state = ELDNodeState::Activate;
}

void ULDNode::Start()
{
	_state = ELDNodeState::Running;
	
	for (auto script : ConditionScripts) {
		script->End();
	}

	Execute();
}

void ULDNode::Failure()
{
	_state = ELDNodeState::Failure;

	for (auto script : ConditionScripts) {
		script->End();
	}
}

void ULDNode::Execute()
{
}

void ULDNode::Deactivate()
{
	_state = ELDNodeState::Deactivate;
}

bool ULDNode::Update(uint32 dt)
{
	if (_state == ELDNodeState::Activate) {

		if (_delayTimer > 0) {
			_delayTimer -= dt;
			return false;
		}

		if (UpdateConditions(dt)) {
			Start();
		}
	}

	if (_state == ELDNodeState::Running) {
		if (UpdateActions(dt)) {
			return End();
		}
	}

	return _state == ELDNodeState::Failure;
}

bool ULDNode::UpdateConditions(uint32 dt)
{
	bool completed = true;

	for (auto& script : ConditionScripts) {

		if (script->_state == ELDNodeState::Deactivate) {
			continue;
		}

		if (script->_delayTimer > 0) {
			script->_delayTimer -= dt;
			completed = false;
			continue;
		}

		if (script->GetState() == ELDNodeState::None) {
			script->Start();
		}

		if (script->Update(dt) == script->Inverted) {
			completed = false;
			if (bEvalCondStartsOnly) {
				Failure();
				break;
			}
			continue;
		}

		if (script->End() == false) {
			completed = false;
		}
	}
	return completed;
}

bool ULDNode::End()
{
	if (_repeatCount++ < Repeat) {
		Reset();
		return false;
	}

	return true;
}

void ULDNode::Reset()
{
	_state = ELDNodeState::Activate;
	_delayTimer = Delay;

	for (auto one : ConditionScripts) {
		one->Reset();
	}

	for (auto one : ActionScripts) {
		one->Reset();
	}
}

bool ULDNode::IsActivable()
{
	if (_state != ELDNodeState::None) {
		return false;
	}

	if (TransitionRule_OR) {
		return true;
	}

	bool running = false;

	for (auto one : _parentNodes) {
		if (one->_state != ELDNodeState::Deactivate && 
			one->_state != ELDNodeState::Failure) {
			running = true;
			break;
		}
	}

	return running == false;
}

void ULDNode::GetNextNodes(TArray<ULDNode*>& output)
{
	checkf(_state != ELDNodeState::Failure, TEXT("State is failure. Override 'GetNextNodes' function for branch your flows."));
	for (auto one : _childNodes) {
		output.Emplace(one);
	}
}

#if WITH_EDITOR

ULDNode* ULDNode::GetRootNode() const
{
	ULDNode* pivotNode = _parentNodes[0];
	while (IsValid(pivotNode) && IsValid(pivotNode->GetParentNode())) {
		pivotNode = pivotNode->GetParentNode();
	}
	return pivotNode;
}

ULDNode* ULDNode::GetParentNode() const
{
	return _parentNodes.Num() > 0 ? _parentNodes[0] : nullptr;
}

void ULDNode::SanitizeParentNodes()
{
	for (int32 i = _parentNodes.Num() - 1; i >= 0; i--) {
		if (_parentNodes[i] == nullptr) { 
			_parentNodes.RemoveAt(i);
		}
	}
}

void ULDNode::AddParentNode(ULDNode* node)
{
	SanitizeParentNodes();

	_parentNodes.AddUnique(node);
}

void ULDNode::SetParentNode(ULDNode* node)
{
	_parentNodes.Empty();
	_parentNodes.Add(node);
}

void ULDNode::AddChildNode(ULDNode* node)
{
	if (IsValid(node)) {
		_childNodes.AddUnique(node);
		node->AddParentNode(this);
	}
}

void ULDNode::RemoveChildNode(ULDNode* node)
{
	if (IsValid(node) && this != nullptr) {
		_childNodes.Remove(node);
		node->_parentNodes.Remove(this);
	}
}

void ULDNode::ClearChildNodes()
{
	_childNodes.Empty();
}

void ULDNode::RemoveFromParentNodes()
{
	auto parentNodes = MoveTemp(_parentNodes);
	for (auto one : parentNodes) {
		if (one) {
			one->RemoveChildNode(this);
		}
	}
}

bool ULDNode::Validate(FString& error)
{
	for (auto one : ConditionScripts) {
		if (one == nullptr || one->Validate(error) == false) {
			return false;
		}
	}

	for (auto one : ActionScripts) {
		if (one == nullptr || one->Validate(error) == false) {
			return false;
		}
	}

	return true;
}

void ULDNode::PushData(TSharedPtr<FJsonObject>& json)
{
	json->SetNumberField("uid", UniqueId);
	json->SetStringField("type", GetClass()->GetFName().ToString());
	json->SetBoolField("rule", TransitionRule_OR);
	json->SetNumberField("repeat", Repeat);
	json->SetNumberField("delay", Delay);

	// condition
	if (ConditionScripts.Num() > 0) {
		TArray<TSharedPtr<FJsonValue>> jsonValueArray;
		for (auto script : ConditionScripts) {
			TSharedPtr<FJsonObject> obj = MakeShared<FJsonObject>();
			script->PushData(obj);
			jsonValueArray.Emplace(MakeShared<FJsonValueObject>(obj));
		}
		json->SetArrayField("condition", jsonValueArray);
	}

	// action
	if (ActionScripts.Num() > 0) {
		TArray<TSharedPtr<FJsonValue>> jsonValueArray;
		for (auto script : ActionScripts) {
			TSharedPtr<FJsonObject> obj = MakeShared<FJsonObject>();
			script->PushData(obj);
			jsonValueArray.Emplace(MakeShared<FJsonValueObject>(obj));
		}
		json->SetArrayField("action", jsonValueArray);
	}

	// child
	if (_childNodes.Num() > 0) {
		TArray<TSharedPtr<FJsonValue>> jsonValueArray;
		for (auto node : _childNodes) {
			jsonValueArray.Emplace(MakeShared<FJsonValueNumber>(node->UniqueId));
		}
		json->SetArrayField("child", jsonValueArray);
	}
}
#endif