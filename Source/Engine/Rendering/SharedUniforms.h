#pragma once

#include "EngineName.h"

#include <glm/glm.hpp>

namespace SharedUniforms
{
	static EngineName MatricesUniform{ "Matrices" };
	static EngineName LightUniform{ "Light" };
}

struct CameraMatrices
{
	alignas(16) glm::mat4 projection;
	alignas(16) glm::mat4 view;
};
