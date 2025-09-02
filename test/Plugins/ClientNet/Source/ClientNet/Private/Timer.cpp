// Copyright 2020 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "Timer.h"
#include "ClientNet.h"

///////////////////////////////////////////////////////////////////// 
///////////////////////////////////////////////////////////////////// 

FTimerTask::~FTimerTask()
{

}

void FTimerTask::Reset()
{
	task.Reset();
	repeatable.Reset();

	elapsed = 0.f;
}

bool FTimerTask::Tick(float deltaTime)
{
	elapsed += deltaTime;

	if (elapsed < interval) {
		return false;
	}

	return true;
}

bool FTimerTask::Execute()
{
	elapsed = 0.f;

	if (task.IsSet()) {
		task->ExecuteIfBound();
		return false;
	}

	return repeatable.IsSet() ? repeatable->Execute() : false;
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

void FClientNetTimer::AddTask(FName id, float interval, ScheduleTaskDelegate task)
{
	auto& entry = _tasks.FindOrAdd(id);
	entry = MakeShared<FTimerTask>();
	entry->interval = interval;
	entry->task.Emplace(task);
	entry->repeatable.Reset();

	UE_LOG(LogClientNet, Verbose, TEXT("timer task added[%s] total[%d]"), *id.ToString(), _tasks.Num());
}

void FClientNetTimer::AddTask(FName id, float interval, RepeatableTaskDelegate task)
{
	auto& entry = _tasks.FindOrAdd(id);
	entry = MakeShared<FTimerTask>();
	entry->interval = interval;
	entry->repeatable.Emplace(task);
	entry->task.Reset();

	UE_LOG(LogClientNet, Verbose, TEXT("timer task added[%s] total[%d]"), *id.ToString(), _tasks.Num());
}

void FClientNetTimer::Tick(float deltaTime)
{
	TSet<TPair<FName, TWeakPtr<FTimerTask>>> fires;
	for (auto& pair : _tasks) {
		if (pair.Value->Tick(deltaTime)) {
			fires.Add(TPair<FName, TWeakPtr<FTimerTask>>(pair.Key, pair.Value));
		}
	}

	for (auto task : fires) {
		if (auto p = task.Value.Pin()) {
			if (p->Execute() == false) {
				_tasks.Remove(task.Key);
			}
		}
	}
}

void FClientNetTimer::RemoveAll()
{
	_tasks.Empty();
}

void FClientNetTimer::Remove(FName id)
{
	_tasks.Remove(id);
}
