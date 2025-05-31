#include "Camera.h"
#include "Engine.h"
#include "Rendering/RenderingInterface.h"

#include <iostream>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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
	GameEngine->GetRenderingSystem()->onWindowResizeParams.Bind(this, &Camera::HandleWindowResize);
	UpdateVectors();
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

void Camera::UpdateDirections()
{
	glm::quat rotation{ data.eulers };
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
	data.projection = glm::perspectiveRH_ZO(glm::radians(data.fieldOfView), aspectRatio, data.nearView, data.farView);
	data.projection[1][1] *= -1;
}

void Camera::SetOrthographicCamera()
{
	float aspectRatio = GameEngine->GetRenderingSystem()->GetAspectRatio();

	float left = -data.ortographicSize * aspectRatio;
	float right = data.ortographicSize * aspectRatio;

	data.projection = glm::ortho(left, right, -data.ortographicSize, data.ortographicSize, -data.cameraZ, data.cameraZ);
	data.projection[1][1] *= -1; // Flip Y because glm uses Open GL coordinates
}

void Camera::UpdateVectors()
{
	if (data.cameraType == ECameraType::Perspective)
	{
		data.view = glm::lookAt(
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
