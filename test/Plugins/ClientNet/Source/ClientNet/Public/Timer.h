#pragma once

#include "CoreMinimal.h"
#include "Timer.generated.h"

DECLARE_DELEGATE(ScheduleTaskDelegate);
DECLARE_DELEGATE_RetVal(bool, RepeatableTaskDelegate);

USTRUCT()
struct CLIENTNET_API FTimerTask
{
	GENERATED_USTRUCT_BODY()

	float interval = 0.f;
	float elapsed = 0.f;

	TOptional<ScheduleTaskDelegate> task;
	TOptional<RepeatableTaskDelegate> repeatable;

	virtual ~FTimerTask();

	virtual void Reset();
	virtual bool Tick(float deltaTime);
	virtual bool Execute();
};

USTRUCT()
struct CLIENTNET_API FClientNetTimer
{
	GENERATED_USTRUCT_BODY()

	~FClientNetTimer() { RemoveAll(); }

	void AddTask(FName id, float interval, ScheduleTaskDelegate task);
	void AddTask(FName id, float interval, RepeatableTaskDelegate task);

	void Tick(float deltaTime);

	void RemoveAll();
	void Remove(FName id);

private:
	TMap<FName, TSharedPtr<FTimerTask>> _tasks;
};
