#pragma once

#include "AssetManager/UniqueID.h"

#include <limits>
#include <vector>
#include <tuple>

constexpr uint32_t TICK_HANDLE = 1 << 2;
constexpr uint32_t UI_RENDER_HANDLE = 1 << 3;
constexpr uint32_t GC_HANDLE = 1 << 4;

#if WITH_EDITOR
constexpr uint32_t EDITOR_UI_HANDLE = 1 << 8;
#endif

constexpr int32_t DEFAULT_PRIORITY = 100;

/// <summary>
/// A task is a container for a function. Its purpose is to handle different systems calling different functions with different priorities.
/// An example of this is the rendering; The main window needs to be on top of everything so it has the highest priority and then if you need a Z order management it can also be done, but that's done by a different system.
/// The handle is very important, that's because it can register specific functions for specific tasks. And example of this would be the Process task. There's the engine process that ticks every frame, but let's say you need to process something, but not with the main engine process, you cn register the tasks with a custom handle and then call the functions with that handle.
/// </summary>

class ITask
{
public:
	virtual ~ITask() = default;
	virtual void Execute() {};
	virtual void ExecuteWithArgs(void* args) {};

	bool operator<(const ITask& other) const
	{
		if (priority != other.priority)
		{
			return priority < other.priority;
		}

		return uniqueID < other.uniqueID;
	}

	bool operator()(const ITask& lhs, const ITask& rhs) const
	{
		if (lhs.priority != rhs.priority)
		{
			return lhs.priority < rhs.priority;
		}

		return lhs.uniqueID < rhs.uniqueID;
	}

protected:
	ITask(int32_t inPariority, int32_t handle = -1)
		: priority(inPariority), customHandle(handle), uniqueID(IDManager::GenerateGUID())
	{
	}

	long priority = 0;
	bool isAlive = true;
	int32_t customHandle = -1;

	UniqueID uniqueID;

	friend class TaskManager;
};

template <class T>
class MemberTask : public ITask
{
public:
	MemberTask(T* obj, int32_t priority, int32_t handle = -1)
		: ITask(priority, handle), owner(obj)
	{
	}

protected:
	T* owner = nullptr;

	friend class TaskManager;
};

template <typename T>
class MemberFunctionTask : public MemberTask<T>
{
public:
	using MemberFunc = void (T::*)();

	MemberFunctionTask(T* obj, MemberFunc func, int32_t priority, int32_t handle = -1)
		: MemberTask<T>(obj, priority, handle), ownerFunc(func)
	{
	}

	void Execute() override
	{
		(this->owner->*ownerFunc)();
	}

private:
	MemberFunc ownerFunc;
};

template <typename T, typename... Args>
class MemberFunctionTaskWithArgs : public MemberTask<T>
{
public:
	using MemberFuncArgs = void (T::*)(Args...);

	MemberFunctionTaskWithArgs(T* obj, MemberFuncArgs func, int32_t priority, int32_t handle = -1)
		: MemberTask<T>(obj, priority, handle), ownerFunc(func) {}

	void ExecuteWithArgs(void* args) override
	{
		std::apply([this](Args... unpackedArgs)
			{ (this->owner->*ownerFunc)(unpackedArgs...); }, *reinterpret_cast<std::tuple<Args...> *>(args));
	}

private:
	MemberFuncArgs ownerFunc;
};

class TaskManager
{
public:
	static TaskManager& Get();

	void ExecuteTasks(int32_t handle);

	template <typename T>
	void RemoveAllTasks(T* obj)
	{
		while (RemoveTask(obj))
			;
	}

	template <typename... Args>
	void ExecuteTasks(int32_t handle, Args... args);

	template <class T>
	void RegisterTask(T* obj, void (T::* func)(), int32_t handle, int32_t priority = DEFAULT_PRIORITY)
	{
		MemberFunctionTask<T>* newTask = new MemberFunctionTask<T>(obj, func, priority, handle);
		tasks.push_back(newTask);
		Sort();
	}

	template <class T, typename... Args>
	void RegisterTask(T* obj, void (T::* func)(Args...), int32_t handle, int32_t priority = DEFAULT_PRIORITY)
	{
		MemberFunctionTaskWithArgs<T, Args...>* newTask = new MemberFunctionTaskWithArgs<T, Args...>(obj, func, priority, handle);
		tasks.push_back(newTask);
		Sort();
	}

private:
	void Sort();

	template <typename T>
	bool RemoveTask(T* obj)
	{
		auto it = std::find_if(tasks.begin(), tasks.end(), [obj](const ITask* task)
			{
				if (const MemberTask<T>* memberTask = reinterpret_cast<const MemberTask<T>*>(task))
				{
					return memberTask->owner == obj && memberTask->isAlive;
				}
				return false; });

		if (it != tasks.end())
		{
			(*it)->isAlive = false;
			return true;
		}

		return false;
	}

	TaskManager() = default;

	std::vector<ITask*> tasks;

	friend class Engine;
};

template <typename... Args>
inline void TaskManager::ExecuteTasks(int32_t handle, Args... args)
{
	for (auto it = tasks.begin(); it != tasks.end();)
	{
		ITask* task = *it;

		if (task->customHandle != handle)
		{
			++it;
			continue;
		}

		if (task->isAlive)
		{
			std::tuple<Args...> tupleArgs(std::forward<Args>(args)...);
			task->ExecuteWithArgs(&tupleArgs);
			++it;
		}
		else
		{
			it = tasks.erase(it);
		}
	}
}