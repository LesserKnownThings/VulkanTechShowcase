#pragma once

#include "Rendering/AbstractData.h"
#include "Rendering/Descriptors/DescriptorInfo.h"
#include "Rendering/Light/Light.h"

#include <queue>
#include <unordered_map>

struct Light;

class LightSystem
{
public:
	Light CreateDirectionalLight(const glm::vec3& rotation);

	void RotateLight(uint32_t lightID, const glm::vec3& axis, float angle);

private:
	void UpdateLightBuffer(uint32_t index, const LightInstance& lightInstance);

	std::queue<uint32_t> freeHandles;
	std::unordered_map<uint32_t, LightInstance> lights;

	uint32_t lightsCount = 0;
};