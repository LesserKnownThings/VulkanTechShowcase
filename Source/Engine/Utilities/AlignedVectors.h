#pragma once

#include <glm/glm.hpp>

struct AlignedMatrix3
{
	AlignedMatrix3() = default;
	AlignedMatrix3(const glm::mat3& matrix, const glm::vec3& viewPosition)
	{
		cols[0] = glm::vec4(matrix[0], viewPosition.x);
		cols[1] = glm::vec4(matrix[1], viewPosition.y);
		cols[2] = glm::vec4(matrix[2], viewPosition.z);
	}

	alignas(16) glm::vec4 cols[3];
};