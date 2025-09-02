// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once

#include "LevelDesignDefine.h"
#include "LDNode.generated.h"

class ANULEVELDESIGN_API ULDConditionScript;
class ANULEVELDESIGN_API ULDActionScript;

UCLASS(Blueprintable)
class ANULEVELDESIGN_API ULDNode : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "Default")
	int32 UniqueId = 0;
	UPROPERTY(VisibleAnywhere, Category = "Default")
	FName Title;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Default", meta = (MultiLine = true))
	FText Text;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Default")
	bool TransitionRule_OR;
	UPROPERTY(VisibleAnywhere, Category = "Default")
	TArray<ULDNode*> _childNodes;
	UPROPERTY(VisibleAnywhere, Category = "Default")
	TArray<ULDNode*> _parentNodes;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Default")
	int32 Repeat;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Default")
	int32 Delay;


	UPROPERTY(EditAnywhere, Category = "Appearance")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Condition")
	bool bEvalCondStartsOnly = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Condition", Instanced)
	TArray<ULDConditionScript*> ConditionScripts;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Action", Instanced)
	TArray<ULDActionScript*> ActionScripts;

	// runtime
	ELDNodeState _state;
	bool _builded = false;
	int32 _delayTimer = 0;
	int32 _repeatCount = 0;

public:
	virtual bool IsActivable();
	virtual void GetNextNodes(TArray<ULDNode*>& output);

	virtual void Build(FLDBlackboard* blackboard);
	virtual void PopData(FNetPacket* packet);
	virtual void Activate();
	virtual void Deactivate();
	virtual void Start();
	virtual void Failure();
	virtual void Execute();
	virtual bool Update(uint32 dt);

protected:
	virtual bool UpdateConditions(uint32 dt);
	virtual bool UpdateActions(uint32 dt) { return true; }
	virtual bool End();
	virtual void Reset();

public:
	ULDNode();
	void GenerateUniqueId();

#if WITH_EDITOR
public:
	uint32 GetUniqueNodeId() const { return UniqueId; }

	ULDNode* GetRootNode() const;
	ULDNode* GetParentNode() const;

	const TArray<ULDNode*>& GetParentNodes() const { return _parentNodes; }
	const TArray<ULDNode*>& GetChildNodes() const { return _childNodes; }

	void SanitizeParentNodes();
	void AddParentNode(ULDNode* node);
	void SetParentNode(ULDNode* node);

	virtual void AddChildNode(ULDNode* node);
	virtual void RemoveChildNode(ULDNode* node);
	virtual void ClearChildNodes();
	void RemoveFromParentNodes();

	virtual bool Validate(FString& error);
	virtual void PushData(TSharedPtr<FJsonObject>& json);
#endif
};