#pragma once

#include <glm/glm.hpp>

struct SingleEntityConstant
{
	alignas(16) glm::mat4 model;
};

struct LightConstant
{
	alignas(16) glm::mat3 lightModel;
	alignas(16) int32_t lightsCount;
	alignas(16) float ambientStrength;
};