#pragma once

#include "Camera/Camera.h"
#include "Engine.h"
#include "Rendering/RenderingInterface.h"

#include <glm/glm.hpp>
#include <limits>
#include <vector>

namespace LightUtilities
{
	static glm::mat4 GetLightSpaceMatrix(const Camera& camera, const glm::vec3& lightEulers, float nearView, float farView)
	{
		const float aspectRatio = GameEngine->GetRenderingSystem()->GetAspectRatio();

		glm::mat4 projection = glm::perspective(glm::radians(camera.data.fieldOfView), aspectRatio, nearView, farView);

		const auto corners = Camera::GetWorldSpaceFrustumCorners(projection, camera.data.view);
		const glm::vec3 center = Camera::GetWorldSpaceFrustumCenter(corners);

		const glm::quat rot = glm::quat(glm::radians(lightEulers));
		glm::vec3 dir = rot * WORLD_FORWARD;

		const glm::vec3 eye = center - dir;
		const glm::mat4 lightView = glm::lookAtLH(eye, center, WORLD_UP);

		float minX = std::numeric_limits<float>::max();
		float maxX = std::numeric_limits<float>::lowest();
		float minY = std::numeric_limits<float>::max();
		float maxY = std::numeric_limits<float>::lowest();
		float minZ = std::numeric_limits<float>::max();
		float maxZ = std::numeric_limits<float>::lowest();
		for (const auto& v : corners)
		{
			const auto trf = lightView * v;
			minX = std::min(minX, trf.x);
			maxX = std::max(maxX, trf.x);
			minY = std::min(minY, trf.y);
			maxY = std::max(maxY, trf.y);

			minZ = std::min(minZ, trf.z);
			maxZ = std::max(maxZ, trf.z);
		}

		constexpr float zMult = 5.0f;
		if (minZ < 0)
		{
			minZ *= zMult;
		}
		else
		{
			minZ /= zMult;
		}
		if (maxZ < 0)
		{
			maxZ /= zMult;
		}
		else
		{
			maxZ *= zMult;
		}

		const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

		return lightProjection * lightView;
	}

	static std::vector<glm::mat4> GetLightSpaceMatrices(const Camera& camera, const glm::vec3& lightEulers)
	{
		const std::vector<float> shadowCascadeLevels = camera.shadowCascadeLevels;
		const int32_t shadowCascadeLevelsCount = camera.shadowCascadeLevels.size();

		std::vector<glm::mat4> matrices;
		for (int32_t i = 0; i < shadowCascadeLevelsCount + 1; ++i)
		{
			if (i == 0)
			{
				matrices.push_back(GetLightSpaceMatrix(camera, lightEulers, camera.data.nearView, shadowCascadeLevels[i]));
			}
			else if (i < shadowCascadeLevels.size())
			{
				matrices.push_back(GetLightSpaceMatrix(camera, lightEulers, shadowCascadeLevels[i - 1], shadowCascadeLevels[i]));
			}
			else
			{
				matrices.push_back(GetLightSpaceMatrix(camera, lightEulers, shadowCascadeLevels[i - 1], camera.data.farView));
			}
		}

		return matrices;
	}
}