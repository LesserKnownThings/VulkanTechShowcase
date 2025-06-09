#pragma once

#include <glm/glm.hpp>

struct AlignedMatrix3
{
	AlignedMatrix3() = default;
	AlignedMatrix3(const glm::mat3& matrix, const glm::vec3& viewPosition)
	{
		col0 = glm::vec4(matrix[0], viewPosition.x);
		col1 = glm::vec4(matrix[1], viewPosition.y);
		col2 = glm::vec4(matrix[2], viewPosition.z);
	}

	alignas(16) glm::vec4 col0;
	alignas(16) glm::vec4 col1;
	alignas(16) glm::vec4 col2;
};