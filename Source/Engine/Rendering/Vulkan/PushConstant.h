#pragma once

#include "Utilities/AlignedVectors.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

struct SharedConstant
{
	alignas(16) glm::mat4 model;
	alignas(16) AlignedMatrix3 normalMatrix;
	int32_t lightsCount = 0;
	float ambientStrength = 0.0f;
	uint32_t hasAnimations = 0;
};