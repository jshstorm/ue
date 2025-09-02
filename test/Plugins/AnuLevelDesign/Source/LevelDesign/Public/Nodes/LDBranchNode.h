// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LDNode.h"
#include "LDBranchNode.generated.h"

UCLASS(Blueprintable, AdvancedClassDisplay, hidecategories = (Action, Appearance))
class ANULEVELDESIGN_API ULDBranchNode : public ULDNode
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere, Category="Value")
	TMap<int32, bool> WhenConditions;

private:
	bool _failure = false;

public:
	ULDBranchNode();

public:
	virtual void Start() final;
	virtual void Failure() final;
	virtual void GetNextNodes(TArray<ULDNode*>& output) final;

#if WITH_EDITOR
public:
	virtual void AddChildNode(ULDNode* node) final;
	virtual void RemoveChildNode(ULDNode* node) final;
	virtual void ClearChildNodes() final;

	virtual bool Validate(FString& error) final;
	virtual void PushData(TSharedPtr<FJsonObject>& json) final;
#endif
};