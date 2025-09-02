// Copyright 2018 CLOVERGAMES Co., Ltd. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"

class FNetAsyncTask : public FNonAbandonableTask
{
	using AsyncTask = TFunction<void()>;

public:
	AsyncTask _task;

public:
	FNetAsyncTask(AsyncTask task)
	{
		_task = task;
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(WorkerAsyncTask, STATGROUP_ThreadPoolAsyncTasks);
	}

	void DoWork()
	{
		_task();
	}
};
