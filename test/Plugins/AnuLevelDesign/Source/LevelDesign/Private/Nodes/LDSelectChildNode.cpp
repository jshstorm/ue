// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "Nodes/LDSelectChildNode.h"
#include "NetPacket.h"

void ULDSelectChildNode::PopData(FNetPacket* packet)
{
	Super::PopData(packet);

	uint8 count = *packet;
	for (uint8 i = 0; i < count; ++i) {
		uint32 uid = *packet;
		_selected.Emplace(ChildMap[uid]);
	}
	_builded = true;
}

void ULDSelectChildNode::Build(FLDBlackboard* blackboard)
{
	Super::Build(blackboard);

	if (_builded) {
		return;
	}

	if (IndividualOperation) {
		for (auto& pair : Probability) {
			if (FMath::RandRange(0, 10000) <= pair.Value) {
				_selected.Emplace(ChildMap[pair.Key]);
			}
		}
	}
	else {
		int32 sum = 0;
		for (auto& pair : Probability) {
			sum += pair.Value;
		}

		int32 randomValue = FMath::RandRange(0, sum);
		for (auto& pair : Probability) {
			if (randomValue <= pair.Value) {
				_selected.Emplace(ChildMap[pair.Key]);
				break;
			}
			randomValue -= pair.Value;
		}
	}

	_builded = true;
}

void ULDSelectChildNode::GetNextNodes(TArray<ULDNode*>& output)
{
	output = _selected;
}

#if WITH_EDITOR
void ULDSelectChildNode::AddChildNode(ULDNode* node)
{
	Super::AddChildNode(node);

	Probability.Emplace(node->UniqueId, 0);
	ChildMap.Emplace(node->UniqueId, node);
}

void ULDSelectChildNode::RemoveChildNode(ULDNode* node)
{
	Super::RemoveChildNode(node);

	Probability.Remove(node->UniqueId);
	ChildMap.Remove(node->UniqueId);
}

void ULDSelectChildNode::ClearChildNodes()
{
	Super::ClearChildNodes();

	Probability.Empty();
	ChildMap.Empty();
}

bool ULDSelectChildNode::Validate(FString& error)
{
	if (Super::Validate(error) == false) {
		return false;
	}

	if (_childNodes.Num() == 0) {
		return false;
	}

	for (auto& pair : Probability) {
		if (pair.Value < 0) {
			return false;
		}
	}

	return true;
}

void ULDSelectChildNode::PushData(TSharedPtr<FJsonObject>& json)
{
	Super::PushData(json);

	TArray<TSharedPtr<FJsonValue>> jsonValueArray;

	for (auto node : _childNodes) {
		float value = Probability[node->UniqueId];
		jsonValueArray.Emplace(MakeShared<FJsonValueNumber>(value));
	}

	json->SetArrayField("probability", jsonValueArray);
	json->SetBoolField("operation", IndividualOperation);
}
#endif