#pragma once

#include <glm/glm.hpp>

struct CameraMatrices
{
	alignas(16) glm::mat4 projection;
	alignas(16) glm::mat4 view;
};