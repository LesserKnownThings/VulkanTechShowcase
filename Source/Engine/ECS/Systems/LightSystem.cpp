#include "LightSystem.h"
#include "ECS/Components/Components.h"
#include "Engine.h"
#include "Rendering/Descriptors/DescriptorRegistry.h"
#include "Rendering/RenderingInterface.h"
#include "Utilities/Color.h"

Light LightSystem::CreateDirectionalLight(const glm::vec3& rotation)
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

	lights[currentID] = LightInstance{};
	LightInstance& ref = lights[currentID];
	ref.type = LIGHT_TYPE_DIRECTIONAL;
	ref.intensity = 20.0f;
	ref.eulers = rotation;
	Color color = Color::white;
	ref.color = color.GetVector();

	UpdateLightBuffer(currentID, ref);

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

void LightSystem::UpdateLightBuffer(uint32_t index, const LightInstance& lightInstance)
{
	RenderingInterface* renderingInterface = GameEngine->GetRenderingSystem();
	DescriptorRegistry* descriptorRegistry = renderingInterface->GetDescriptorRegistry();

	AllocatedBuffer lightBuffer = descriptorRegistry->GetLightBuffer();

	LightBufferLayout data[1] = { lightInstance.CreateBufferLayout() };
	renderingInterface->UpdateBuffer(lightBuffer, sizeof(LightBufferLayout) * index, sizeof(LightBufferLayout), data);
}
