// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "Nodes/LDBranchNode.h"

ULDBranchNode::ULDBranchNode()
	: Super()
{
	bEvalCondStartsOnly = true;
}

void ULDBranchNode::Start()
{
	Super::Start();
	_failure = false;
}

void ULDBranchNode::Failure()
{
	Super::Failure();
	_failure = true;
}

void ULDBranchNode::GetNextNodes(TArray<ULDNode*>& output)
{
	output = _childNodes;

	const bool isSuccess = !_failure;
	for (int32 i = 0; i < output.Num(); ++i) {
		if (WhenConditions[output[i]->UniqueId] != isSuccess) {
			output.RemoveAt(i--);
			continue;
		}
	}
}

#if WITH_EDITOR
void ULDBranchNode::AddChildNode(ULDNode* node)
{
	Super::AddChildNode(node);

	WhenConditions.Emplace(node->UniqueId, true);
}

void ULDBranchNode::RemoveChildNode(ULDNode* node)
{
	Super::RemoveChildNode(node);

	WhenConditions.Remove(node->UniqueId);
}

void ULDBranchNode::ClearChildNodes()
{
	Super::ClearChildNodes();

	WhenConditions.Empty();
}

bool ULDBranchNode::Validate(FString& error)
{
	if (Super::Validate(error) == false) {
		return false;
	}

	// Branch node need one ore more child nodes.
	if (WhenConditions.Num() == 0) {
		return false;
	}

	return true;
}

void ULDBranchNode::PushData(TSharedPtr<FJsonObject>& json)
{
	Super::PushData(json);

	TArray<TSharedPtr<FJsonValue>> jsonValueArray;

	for (auto node : _childNodes) {
		bool value = WhenConditions[node->UniqueId];
		jsonValueArray.Emplace(MakeShared<FJsonValueBoolean>(value));
	}

	json->SetArrayField("whenConditions", jsonValueArray);
}
#endif