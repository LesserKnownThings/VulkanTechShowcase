#pragma once

#include "Entity.h"
#include "Utilities/Delegate.h"

#include <deque>
#include <vector>

DECLARE_DELEGATE_OneParam(OnEntityRemoved, const Entity&);

class EntityManager
{
public:
	static EntityManager& Get();

	uint32_t GetEntityIndex(const Entity& e) const;
	uint32_t GetEntityGeneration(const Entity& e) const;

	bool IsEntityAlive(const Entity& e) const;

	Entity CreateEntity();
	void DestroyEntity(const Entity& e);

	/**
	 * Should only be used on components that hold resources
	 * If component has no resources leave it to the gc
	 */
	OnEntityRemoved onEntityRemoved;

private:
	EntityManager() = default;

	std::vector<uint32_t> generation;
	std::deque<uint32_t> free_indices;

	friend class Engine;
};
