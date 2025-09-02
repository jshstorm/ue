// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once
#include "LDNode.h"
#include "UObject/NoExportTypes.h"
#include "LDSelectChildNode.generated.h"

UCLASS(Blueprintable, AdvancedClassDisplay, hidecategories = (Action, Appearance))
class ANULEVELDESIGN_API ULDSelectChildNode : public ULDNode
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Value")
	TMap<int32, int32> Probability;
	UPROPERTY()
	TMap<int32, ULDNode*> ChildMap;
	UPROPERTY(EditAnywhere, Category = "Value")
	bool IndividualOperation = false;

private:
	// runtime
	TArray<ULDNode*> _selected;
	bool _builded = false;

public:
	virtual void PopData(FNetPacket* packet) final;
	virtual void Build(FLDBlackboard* blackboard) final;
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