#include "TaskManager.h"

#include <algorithm>

TaskManager& TaskManager::Get()
{
	static TaskManager instance;
	return instance;
}

void TaskManager::ExecuteTasks(int32_t handle)
{
	for (auto it = tasks.begin(); it != tasks.end();)
	{
		ITask *task = *it;

		if (task->customHandle != handle)
		{
			++it;
		}
		else
		{
			if (task->isAlive)
			{
				task->Execute();
				++it;
			}
			else
			{
				delete task;
				it = tasks.erase(it);
			}
		}
	}
}

void TaskManager::Sort()
{
	std::sort(tasks.begin(), tasks.end(), [](const ITask* a, const ITask* b) {
		return a->priority < b->priority;
		});
}
