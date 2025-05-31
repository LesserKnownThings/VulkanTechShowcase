#pragma once

#include "Rendering/AbstractData.h"
#include "Rendering/Descriptors/DescriptorInfo.h"
#include "Rendering/Light/Light.h"
#include "SystemBase.h"

#include <entt/entity/registry.hpp>
#include <unordered_map>

struct Light;
struct CommonLightData;
struct PointLightData;
struct SpotLightData;

class LightSystem : public SystemBase
{
public:
	LightSystem(const entt::registry& inRegistry);

	Light CreateLight(const entt::entity& entity, CommonLightData* lightData, ELightType lightType);

	void RotateLight(uint32_t lightID, const glm::vec3& axis, float angle);

	void UpdateLightPosition(uint32_t lightHandle, const glm::vec3& position);
	void UpdateLightSubBuffer(uint32_t lightHandle, uint32_t offset, uint32_t size);

private:
	Light CreateDirectionalLight(const entt::entity& entity, CommonLightData* data);
	Light CreatePointLight(const entt::entity& entity, PointLightData* data);
	Light CreateSpotLight(const entt::entity& entity, SpotLightData* data);

	void UpdateLightSubBuffer(LightInstance& instance, uint32_t handle, uint32_t offset, uint32_t size);
	void UpdateLightBuffer(uint32_t index, const LightInstance& lightInstance);

	const entt::registry& registry;

	std::unordered_map<uint32_t, LightInstance> lights;
};