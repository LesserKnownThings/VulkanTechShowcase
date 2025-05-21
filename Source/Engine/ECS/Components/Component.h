#pragma once

#include <cstdint>

struct Entity;

enum class EComponentState : uint8_t
{
	None,
	Loading,
	Ready,
	Dead
};

struct Component
{
	int32_t instances = 0;
	int32_t allocatedInstances = 0;

	void* buffer = nullptr;
	Entity* entity = nullptr;
	EComponentState* state = nullptr;
};