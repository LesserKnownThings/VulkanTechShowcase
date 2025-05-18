#include "SystemBase.h"
#include "ECS/Entity.h"
#include "ECS/EntityManager.h"
#include "TaskManager.h"

void SystemBase::Initialize()
{
	EntityManager::Get().onEntityRemoved.Bind(this, &SystemBase::HandleEntityRemoved);
	TaskManager::Get().RegisterTask(this, &SystemBase::GCPass, GC_HANDLE);
	Allocate(DEFAULT_ALLOCATED_MEMORY);
}

void SystemBase::GCPass()
{
	for (int32_t i = 0; i < pendingDeletion.size(); ++i)
	{
		DestroyComponent(pendingDeletion[i]);
	}
	pendingDeletion.clear();
}
