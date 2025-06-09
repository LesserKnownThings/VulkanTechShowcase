#pragma once

#include "Camera/Camera.h"
#include "Rendering/AbstractData.h"
#include "Utilities/Color.h"

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <iostream>

constexpr uint32_t MAX_LIGHTS = 50;

enum class ELightType : uint32_t
{
	Directional = 1 << 0,
	Point = 1 << 1,
	Spot = 1 << 2
};

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

struct CommonLightData
{
	Color color;
	float intensity;
};

struct PointLightData : public CommonLightData
{
	float range;
};

struct SpotLightData : public CommonLightData
{
	float range;
	float angle;	
};

struct LightInstance
{
	glm::vec3 position;
	glm::vec3 eulers;
	glm::vec3 color;
	uint32_t type = 0;
	float intensity = 1.0f;
	float range = 0.0f;
	float angle = 0.0f;

	LightBufferLayout CreateBufferLayout() const
	{
		LightBufferLayout bufferLayout{};
		bufferLayout.position = glm::vec4(position, glm::uintBitsToFloat(type));

		const glm::quat rot = glm::quat(glm::radians(eulers));
		glm::vec3 dir = rot * WORLD_FORWARD;

		bufferLayout.direction = glm::vec4(dir, 0.0f);
		bufferLayout.color = glm::vec4(color, intensity);
		constexpr float OUT_CUTOFF = 5.0f;
		const float outerCutOff = angle + OUT_CUTOFF;
		bufferLayout.params = glm::vec4(range, glm::cos(glm::radians(angle)), glm::cos(glm::radians(outerCutOff)), 0.0f);

		return bufferLayout;
	}
};
