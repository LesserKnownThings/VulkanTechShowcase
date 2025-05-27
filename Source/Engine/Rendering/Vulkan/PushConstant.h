#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

struct alignas(16) EntityTransformModel
{
	glm::mat4 model;
};

struct AlignedMatrix3
{
	AlignedMatrix3() = default;
	AlignedMatrix3(const glm::mat3& matrix)
	{
		cols[0] = glm::vec4(matrix[0], 0.0f);
		cols[1] = glm::vec4(matrix[1], 0.0f);
		cols[2] = glm::vec4(matrix[2], 0.0f);
	}

	alignas(16) glm::vec4 cols[3];
};

struct LightConstant
{
	AlignedMatrix3 normalMatrix;
	int32_t lightsCount;
	float ambientStrength;
};