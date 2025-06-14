#pragma once

#include "Rendering/AbstractData.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Transform
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 eulers = glm::vec3(0.0f);
	glm::vec3 scale = glm::vec3(1.0f);

	// TODO move these to a system instead of the component

	glm::mat4 ComputeModel() const
	{
		glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
		model = glm::rotate(model, glm::radians(eulers.x), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(eulers.y), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(eulers.z), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, scale);
		return model;
	}

	glm::mat3 ComputeNormalMatrix() const
	{
		return glm::transpose(glm::inverse(glm::mat3(ComputeModel())));
	}
};

struct ModelComponent
{
	uint32_t handle = 0;
};

struct Light
{
	uint32_t lightInstanceHandle = 0;
};

struct AnimatorComponent
{
	uint32_t animatorInstanceHandle = 0;
};