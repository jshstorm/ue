// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "LDObjectiveScript.h"

ULDObjectiveScript::ULDObjectiveScript()
{
	GenerateUniqueId();
}

void ULDObjectiveScript::GenerateUniqueId()
{
	UniqueId = GetUniqueID();
}

void ULDObjectiveScript::PopData(FNetPacket* packet)
{
}

void ULDObjectiveScript::Build(FLDBlackboard* blackboard)
{
	_blackboard = blackboard;
	_delayTimer = Delay;
}

void ULDObjectiveScript::Start()
{
#if DEBUG_LD
	UE_LOG(LogLD, Verbose, TEXT("[LD] Script : [%d]"), UniqueId);
#endif

	_state = ELDNodeState::Running;

	InitializeStageEventHandler();

	if (_handlers.Num() > 0) {
		_blackboard->Register.ExecuteIfBound(this);
	}
}

bool ULDObjectiveScript::End()
{
	if (_repeatCount < Repeat) {
		_state = ELDNodeState::None;
		_delayTimer = Delay;
		++_repeatCount;
	}

	if (_repeatCount < Repeat) {
		return false;
	}

	_state = ELDNodeState::Deactivate;

	if (_handlers.Num() > 0) {
		_blackboard->Unregister.ExecuteIfBound(this);
	}

	return true;
}

void ULDObjectiveScript::ForceEnd()
{
	_repeatCount = Repeat;

	End();
}

void ULDObjectiveScript::Reset()
{
	_state = ELDNodeState::None;
	_delayTimer = 0;
	_repeatCount = 0;
}

void ULDObjectiveScript::PopProxyData(FNetPacket* packet)
{
}

void ULDObjectiveScript::PushProxyData(FNetPacket* packet)
{
}

void ULDObjectiveScript::RequestUpdate(FNetPacket* packet)
{
	ProxyUpdateDelegate.ExecuteIfBound(packet);
}

#if WITH_EDITOR
void ULDObjectiveScript::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

	UniqueId = GetUniqueID();
}

bool ULDObjectiveScript::Validate(FString& error)
{
	return true;
}

void ULDObjectiveScript::PushData(TSharedPtr<FJsonObject>& json)
{
	json->SetNumberField("uid", UniqueId);
	json->SetStringField("type", GetClass()->GetFName().ToString());
	json->SetNumberField("delay", Delay);
	json->SetNumberField("repeat", Repeat);
}
#endif