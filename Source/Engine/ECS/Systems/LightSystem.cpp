#include "LightSystem.h"
#include "ECS/Components/Components.h"

/// <summary>
/// I'm using MSB for light type flag in the handle
/// 28 bits is more than enough for light IDs
/// </summary>
constexpr uint32_t DIRECTIONAL_LIGHT_FLAG = 1u << 31;
constexpr uint32_t POINT_LIGHT_FLAG = 1u << 30;
constexpr uint32_t SPOT_LIGHT_FLAG = 1u << 29;

Light LightSystem::CreateDirectionalLight(const glm::vec3& direction)
{
	uint32_t currentID = 0;
	if (!freeHandles.empty())
	{
		currentID = freeHandles.front();
		freeHandles.pop();
		
		currentID &= ~POINT_LIGHT_FLAG;
		currentID &= ~SPOT_LIGHT_FLAG;

		currentID |= DIRECTIONAL_LIGHT_FLAG;
		lightsCount++;
	}
	else
	{
		currentID = lightsCount++;
	}

	return Light();
}
