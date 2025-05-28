#include "LightSystem.h"
#include "ECS/Components/Components.h"
#include "Engine.h"
#include "Rendering/Descriptors/DescriptorRegistry.h"
#include "Rendering/RenderingInterface.h"
#include "Utilities/Color.h"

#include <iostream>

LightSystem::LightSystem(const entt::registry& inRegistry)
	: registry(inRegistry)
{
}

Light LightSystem::CreateLight(const entt::entity& entity, CommonLightData* lightData, ELightType lightType)
{
	switch (lightType)
	{
	case ELightType::Point:
		return CreatePointLight(entity, reinterpret_cast<PointLightData*>(lightData));
	case ELightType::Spot:
		return CreateSpotLight(entity, reinterpret_cast<SpotLightData*>(lightData));
	default:
		return CreateDirectionalLight(entity, lightData);
	}
}

Light LightSystem::CreateDirectionalLight(const entt::entity& entity, CommonLightData* data)
{
	uint32_t currentID = GenerateLightHandle();

	if (auto transform = registry.try_get<Transform>(entity))
	{
		lights[currentID] = LightInstance{};
		LightInstance& ref = lights[currentID];
		ref.eulers = transform->eulers;
		ref.type = static_cast<uint32_t>(ELightType::Directional);
		ref.intensity = data->intensity;		
		ref.color = data->color.GetVector();

		UpdateLightBuffer(currentID, ref);
	}
	else
	{
		std::cerr << "Failed to get the transform of the light, did you forget to add a transform before creating the light?" << std::endl;
	}

	return Light{ currentID };
}

Light LightSystem::CreatePointLight(const entt::entity& entity, PointLightData* data)
{
	uint32_t currentID = GenerateLightHandle();

	if (auto transform = registry.try_get<Transform>(entity))
	{
		lights[currentID] = LightInstance{};
		LightInstance& ref = lights[currentID];
		ref.type = static_cast<uint32_t>(ELightType::Point);
		ref.position = transform->position;
		ref.intensity = data->intensity;
		ref.range = data->range;
		ref.color = data->color.GetVector();

		UpdateLightBuffer(currentID, ref);
	}
	else
	{
		std::cerr << "Failed to get the transform of the light, did you forget to add a transform before creating the light?" << std::endl;
	}

	return Light{ currentID };
}

Light LightSystem::CreateSpotLight(const entt::entity& entity, SpotLightData* data)
{
	uint32_t currentID = GenerateLightHandle();

	if (auto transform = registry.try_get<Transform>(entity))
	{
		lights[currentID] = LightInstance{};
		LightInstance& ref = lights[currentID];
		ref.type = static_cast<uint32_t>(ELightType::Spot);
		ref.position = transform->position;
		ref.eulers = transform->eulers;
		ref.intensity = data->intensity;
		ref.range = data->range;
		ref.angle = data->angle;
		ref.color = data->color.GetVector();

		UpdateLightBuffer(currentID, ref);
	}
	else
	{
		std::cerr << "Failed to get the transform of the light, did you forget to add a transform before creating the light?" << std::endl;
	}

	return Light{ currentID };
}

void LightSystem::RotateLight(uint32_t lightID, const glm::vec3& axis, float angle)
{
	auto it = lights.find(lightID);
	if (it != lights.end())
	{
		LightInstance& instance = it->second;
		glm::vec3 rot = glm::vec3(instance.eulers);
		rot += axis * angle;
		rot = glm::mod(rot, 360.0f);
		instance.eulers = rot;

		UpdateLightBuffer(lightID, instance);
	}
}

void LightSystem::UpdateLightPosition(uint32_t lightHandle, const glm::vec3& position)
{
	auto it = lights.find(lightHandle);
	if (it != lights.end())
	{
		LightInstance& instance = it->second;
		instance.position = position;
		UpdateLightSubBuffer(instance, lightHandle, offsetof(LightBufferLayout, position), sizeof(glm::vec4));
	}
}

void LightSystem::UpdateLightSubBuffer(uint32_t lightHandle, uint32_t offset, uint32_t size)
{
	auto it = lights.find(lightHandle);
	if (it != lights.end())
	{
		LightInstance& instance = it->second;
		UpdateLightSubBuffer(instance, lightHandle, offset, size);
	}
}

void LightSystem::UpdateLightSubBuffer(LightInstance& instance, uint32_t handle, uint32_t offset, uint32_t size)
{
	LightBufferLayout data = instance.CreateBufferLayout();

	RenderingInterface* renderingInterface = GameEngine->GetRenderingSystem();
	DescriptorRegistry* descriptorRegistry = renderingInterface->GetDescriptorRegistry();
	AllocatedBuffer lightBuffer = descriptorRegistry->GetLightBuffer();

	renderingInterface->UpdateBuffer(lightBuffer, sizeof(LightBufferLayout) * handle + offset, size, &data);
}

void LightSystem::UpdateLightBuffer(uint32_t index, const LightInstance& lightInstance)
{
	RenderingInterface* renderingInterface = GameEngine->GetRenderingSystem();
	DescriptorRegistry* descriptorRegistry = renderingInterface->GetDescriptorRegistry();

	AllocatedBuffer lightBuffer = descriptorRegistry->GetLightBuffer();

	LightBufferLayout data = lightInstance.CreateBufferLayout();
	renderingInterface->UpdateBuffer(lightBuffer, sizeof(LightBufferLayout) * index, sizeof(LightBufferLayout), &data);
}

uint32_t LightSystem::GenerateLightHandle()
{
	uint32_t currentID = 0;
	if (!freeHandles.empty())
	{
		currentID = freeHandles.front();
		freeHandles.pop();
		lightsCount++;
	}
	else
	{
		// TODO add a check for max light count reached
		currentID = lightsCount++;
	}
	return currentID;
}
