#include "Camera.h"
#include "Engine.h"
#include "Rendering/RenderingInterface.h"

#include <iostream>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Utilities
{
	glm::vec2 GetExactDirection(glm::vec2 A, glm::vec2 B)
	{
		glm::vec2 direction = B - A;
		return glm::vec2(
			(direction.x == 0) ? 0 : glm::sign(direction.x),
			(direction.y == 0) ? 0 : glm::sign(direction.y));
	}
}

Camera::~Camera()
{
	GameEngine->GetRenderingSystem()->onWindowResizeParams.Clear(this);
}

Camera::Camera()
{
	Initialize();
}

Camera::Camera(const CameraData& inData)
	: data(inData)
{
	Initialize();
}

void Camera::Initialize()
{
	shadowCascadeLevels.resize(4);
	int32_t numCascades = shadowCascadeLevels.size();

	float lambda = 0.5; // between 0 (linear) and 1 (logarithmic)
	for (int i = 0; i < numCascades; ++i) {
		float id = (i + 1) / float(numCascades);
		float log = data.nearView * pow(data.farView / data.nearView, id);
		float lin = data.nearView + (data.farView - data.nearView) * id;
		shadowCascadeLevels[i] = glm::mix(lin, log, lambda);
	}

	/*shadowCascadeLevels[0] = data.farView * 0.05f;
	shadowCascadeLevels[1] = data.farView * 0.15f;
	shadowCascadeLevels[2] = data.farView * 0.40f;
	shadowCascadeLevels[3] = data.farView * 0.90f;*/

	GameEngine->GetRenderingSystem()->onWindowResizeParams.Bind(this, &Camera::HandleWindowResize);
	UpdateDirections();
	UpdateProjectionType();
}

void Camera::SetPosition(const glm::vec3& newPosition)
{
	data.position = newPosition;
	UpdateVectors();
}

void Camera::SetRotation(const glm::vec3& eulerAngles)
{
	data.eulers = eulerAngles;
	UpdateDirections();
}

void Camera::Rotate(const glm::vec3& axis, const glm::vec2& pitchClamp)
{
	data.eulers.x += axis.x;
	data.eulers.x = glm::clamp(data.eulers.x, pitchClamp.x, pitchClamp.y);

	data.eulers.y += axis.y;
	if (std::abs(data.eulers.y) > 10000.0f)
	{
		data.eulers.y = std::fmodf(data.eulers.y, 360.0f);
	}

	UpdateDirections();
}

std::vector<glm::vec4> Camera::GetWorldSpaceFrustumCorners(const glm::mat4& projection, const glm::mat4& view)
{
	const glm::mat4 inv = glm::inverse(projection * view);

	std::vector<glm::vec4> corners;
	for (uint32_t x = 0; x < 2; ++x)
	{
		for (uint32_t y = 0; y < 2; ++y)
		{
			for (uint32_t z = 0; z < 2; ++z)
			{
				glm::vec4 ndc =
				{
					2.0f * x - 1.0f,
					2.0f * y - 1.0f,
					z,
					1.0f
				};
				glm::vec4 worldCoords = inv * ndc;
				corners.emplace_back(worldCoords / worldCoords.w);
			}
		}
	}

	return corners;
}

glm::vec3 Camera::GetWorldSpaceFrustumCenter(const std::vector<glm::vec4>& corners)
{
	glm::vec3 center{};
	for (const glm::vec4& corner : corners)
	{
		center += glm::vec3(corner);
	}
	center /= corners.size();

	return center;
}

void Camera::UpdateDirections()
{
	glm::quat rotation{ glm::radians(data.eulers) };
	data.forward = glm::normalize(rotation * WORLD_FORWARD);
	data.up = glm::normalize(rotation * WORLD_UP);
	data.right = glm::normalize(rotation * WORLD_RIGHT);
	UpdateVectors();
}

void Camera::HandleWindowResize(float width, float height)
{
	UpdateProjectionType();
}

void Camera::UpdateProjectionType()
{
	switch (data.cameraType)
	{
	case ECameraType::Orthographic:
		SetOrthographicCamera();
		break;

	default:
		SetPerspectiveCamera();
		break;
	}

	projectionChanged = true;
}

void Camera::SetPerspectiveCamera()
{
	const float aspectRatio = GameEngine->GetRenderingSystem()->GetAspectRatio();
	data.projection = glm::perspective(glm::radians(data.fieldOfView), aspectRatio, data.nearView, data.farView);
}

void Camera::SetOrthographicCamera()
{
	float aspectRatio = GameEngine->GetRenderingSystem()->GetAspectRatio();

	float left = -data.ortographicSize * aspectRatio;
	float right = data.ortographicSize * aspectRatio;

	data.projection = glm::ortho(left, right, -data.ortographicSize, data.ortographicSize, -data.cameraZ, data.cameraZ);
}

void Camera::UpdateVectors()
{
	if (data.cameraType == ECameraType::Perspective)
	{
		data.view = glm::lookAtLH(
			data.position,
			data.position + data.forward,
			data.up);
	}
	else
	{
		data.view = glm::translate(glm::mat4(1.0f), data.position);
	}

	viewChanged = true;
}
