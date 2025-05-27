#pragma once

#include "Rendering/AbstractData.h"

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <iostream>

constexpr uint32_t MAX_LIGHTS = 50;

constexpr uint32_t LIGHT_TYPE_DIRECTIONAL = 1 << 0;
constexpr uint32_t LIGHT_TYPE_POINT = 1 << 1;
constexpr uint32_t LIGHT_TYPE_SPOT = 1 << 2;

struct LightBufferLayout
{
	// w => type
	alignas(16) glm::vec4 position = glm::vec4(0.0f);
	alignas(16) glm::vec4 direction = glm::vec4(0.0f);
	// a => intensity
	alignas(16) glm::vec4 color = glm::vec4(0.0f);
	// x => range (for point/spot), y => inner cone, z => outer cone
	alignas(16) glm::vec4 params = glm::vec4(0.0f);
};

struct LightInstance
{
	glm::vec3 position;
	glm::vec3 eulers;
	glm::vec3 color;
	uint32_t type = 0;
	float intensity = 1.0f;
	float range = 0.0f;
	float innerCone = 0.0f;
	float outerCone = 0.0f;

	LightBufferLayout CreateBufferLayout() const
	{
		LightBufferLayout bufferLayout{};
		bufferLayout.position = glm::vec4(position, glm::uintBitsToFloat(type));

		const glm::quat rot = glm::quat(glm::radians(eulers));
		glm::vec3 dir = rot * glm::vec3(0.0f, 0.0f, -1.0f);
		dir.y *= -1.0f;

		std::cout << "Light direction: " << dir.x << ", " << dir.y << ", " << dir.z << std::endl;


		bufferLayout.direction = glm::vec4(glm::normalize(dir), 0.0f);
		bufferLayout.color = glm::vec4(color, intensity);
		bufferLayout.params = glm::vec4(range, innerCone, outerCone, 0.0f);

		return bufferLayout;
	}
};
