#pragma once

#include "Component.h"

#include <glm/glm.hpp>

struct MaterialComponent : public Component
{
	glm::vec4* color = nullptr;
	uint32_t* materialHandle = nullptr;
	uint8_t* pipeline = nullptr;
};