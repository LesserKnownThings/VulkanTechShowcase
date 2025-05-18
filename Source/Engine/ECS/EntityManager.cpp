#include "EntityManager.h"

#include <assert.h>

constexpr uint32_t INDEX_BITS = 24;
constexpr uint32_t INDEX_MASK = (static_cast<uint32_t>(1) << INDEX_BITS) - static_cast<uint32_t>(1);

constexpr uint32_t GENERATION_BITS = 8;
constexpr uint32_t GENERATION_MASK = (static_cast<uint32_t>(1) << GENERATION_BITS) - static_cast<uint32_t>(1);

constexpr uint16_t MIN_FREE_INDICES = 1024;

EntityManager& EntityManager::Get()
{
	static EntityManager instance;
	return instance;
}

uint32_t EntityManager::GetEntityIndex(const Entity& e) const
{
	return e.id & INDEX_MASK;
}

uint32_t EntityManager::GetEntityGeneration(const Entity& e) const
{
	return (e.id >> INDEX_BITS) & GENERATION_MASK;
}

bool EntityManager::IsEntityAlive(const Entity& e) const
{
	return generation[GetEntityIndex(e)] == GetEntityGeneration(e);
}

Entity EntityManager::CreateEntity()
{
	uint32_t index;

	if (free_indices.size() > MIN_FREE_INDICES)
	{
		index = free_indices.front();
		free_indices.pop_front();
	}
	else
	{
		generation.push_back(0);
		index = generation.size() - 1;

		assert(index < (static_cast<uint32_t>(1) << INDEX_BITS));
	}

	return Entity{ (generation[index] << INDEX_BITS) | index };
}

void EntityManager::DestroyEntity(const Entity& e)
{
	const uint32_t index = GetEntityIndex(e);
	generation[index]++;
	free_indices.push_back(index);
	onEntityRemoved.Invoke(e);
}
