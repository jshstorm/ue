// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LevelDesignDefine.h"
#include "UObject/NoExportTypes.h"
#include "LDObjectiveScript.generated.h"

class ANULEVELDESIGN_API ULDNode;

UCLASS(abstract, Blueprintable/*, DefaultToInstanced, EditInlineNew*/)
class ANULEVELDESIGN_API ULDObjectiveScript : public UStageEventListener
{
	GENERATED_BODY()

	friend class ULDNode;
	friend class ULDSequenceNode;
	friend class ULDParallelNode;

public:
	UPROPERTY()
	ULDNode* ParentNode;

	UPROPERTY(VisibleAnywhere, Category = "Value")
	uint32 UniqueId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value")
	int32 Delay;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Value")
	int32 Repeat;

public:
	FProxyUpdateDelegate ProxyUpdateDelegate;

protected:
	// runtime
	FLDBlackboard* _blackboard = nullptr;
	ELDNodeState _state = ELDNodeState::None;
	int32 _delayTimer = 0;
	int32 _repeatCount = 0;

public:
	ELDNodeState GetState() { return _state; }
	bool IsBuilded() { return _blackboard != nullptr; }

	virtual void PopData(FNetPacket* packet);
	virtual void Build(FLDBlackboard* blackboard);
	virtual void Start();
	virtual bool End();
	virtual bool Update(uint32 dt) { return true; }
	virtual void Reset();

	virtual void PopProxyData(FNetPacket* packet);
	virtual void PushProxyData(FNetPacket* packet);

	void ForceEnd();

protected:
	virtual void InitializeStageEventHandler() {}
	void RequestUpdate(FNetPacket* packet);

public:
	ULDObjectiveScript();
	void GenerateUniqueId();

#if WITH_EDITOR
public:
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	virtual bool Validate(FString& error);
	virtual void PushData(TSharedPtr<FJsonObject>& json);
#endif
};