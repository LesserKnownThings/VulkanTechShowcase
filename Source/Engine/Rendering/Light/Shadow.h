#pragma once

#include <cstdint>
#include <glm/glm.hpp>

constexpr uint32_t MAX_SM = 16;

struct ShadowLayout
{
	alignas(16) glm::mat4 lightMatrix[MAX_SM];
};