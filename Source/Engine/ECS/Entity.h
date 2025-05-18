#pragma once

#include <cstdint>
#include <vector>

constexpr int32_t DEFAULT_ALLOCATED_MEMORY = 100;

struct Entity
{
	Entity() {}
	Entity(uint32_t inId)
		: id(inId) {}

	bool operator==(const Entity& e) const
	{
		return id == e.id;
	}

	uint32_t id;
};

template <>
struct std::hash<Entity>
{
	size_t operator()(const Entity& e) const
	{
		return std::hash<uint32_t>()(e.id);
	}
};