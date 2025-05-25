#pragma once

#include "Rendering/AbstractData.h"

#include <cstdint>
#include <glm/glm.hpp>

constexpr uint32_t MAX_LIGHTS = 50;

struct LightBufferLayout
{
	// w => type
	alignas(16) glm::vec4 position;
	alignas(16) glm::vec4 direction;
	// a => intensity
	alignas(16) glm::vec4 color;
	// x => range (for point/spot), y => inner cone, z => outer cone
	alignas(16) glm::vec4 params;
};

struct LightInstance
{
	AllocatedBuffer buffer;
	LightBufferLayout layout;
};
