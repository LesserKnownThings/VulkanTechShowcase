#include "Camera.h"
#include "Engine.h"

#include "Rendering/RenderingInterface.h"
#include "TaskManager.h"

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
	TaskManager::Get().RemoveAllTasks(this);
}

Camera::Camera(const glm::vec3& initialPosition, const glm::vec3& initialRotation)
	: position(initialPosition)
{
	GameEngine->GetRenderingSystem()->onWindowResizeParams.Bind(this, &Camera::HandleWindowResize);
	TaskManager::Get().RegisterTask(this, &Camera::Tick, TICK_HANDLE);

	SetRotation(initialRotation);

	UpdateProjectionType();
	UpdateVectors();	
}

void Camera::ChangeType(ECameraType inType)
{
	cameraType = inType;

	UpdateProjectionType();
}

void Camera::SetFieldOfView(float inFieldOfView)
{
	if (cameraType == ECameraType::Perspective)
	{
		fieldOfView = inFieldOfView;
	}

	UpdateProjectionType();
}

void Camera::SetNearView(float inNearView)
{
	if (cameraType == ECameraType::Perspective)
	{
		nearView = inNearView;
	}

	UpdateProjectionType();
}

void Camera::SetFarView(float inFarView)
{
	if (cameraType == ECameraType::Perspective)
	{
		farView = inFarView;
	}

	UpdateProjectionType();
}

void Camera::SetOrthographicSize(float inOrthographicSize)
{
	if (cameraType == ECameraType::Orthographic)
	{
		ortographicSize = inOrthographicSize;
	}

	UpdateProjectionType();
}

void Camera::SetCameraZ(float inCameraZ)
{
	if (cameraType == ECameraType::Orthographic)
	{
		cameraZ = inCameraZ;
	}

	UpdateProjectionType();
}

void Camera::SetPosition(const glm::vec3& newPosition)
{
	position = newPosition;
	UpdateVectors();
}

void Camera::SetRotation(const glm::vec3& eulerAngles)
{
	pitch = eulerAngles.x;
	yaw = eulerAngles.y;
	roll = eulerAngles.z;

	rotation = glm::normalize(glm::quat(glm::radians(glm::vec3(pitch, yaw, roll))));
	UpdateDirections();
}

void Camera::Rotate(const glm::vec3& axis, const glm::vec2& pitchClamp)
{
	pitch += axis.x;
	pitch = glm::clamp(pitch, pitchClamp.x, pitchClamp.y);
	yaw += axis.y;
	roll += axis.z;

	rotation = glm::normalize(glm::quat(glm::radians(glm::vec3(pitch, yaw, roll))));
	UpdateDirections();
}

void Camera::UpdateDirections()
{
	forward = glm::normalize(rotation * glm::vec3(0.f, 0.f, -1.f));
	up = glm::normalize(rotation * glm::vec3(0.f, 1.f, 0.f));
	right = glm::normalize(rotation * glm::vec3(1.f, 0.f, 0.f));
}

void Camera::HandleWindowResize(float width, float height)
{
	UpdateProjectionType();
}

const glm::vec3 Camera::GetEulers() const
{
	return glm::degrees(glm::eulerAngles(rotation));
}

const glm::vec3& Camera::GetForwardVector() const
{
	return forward;
}

const glm::vec3& Camera::GetRightVector() const
{
	return right;
}

const glm::vec3& Camera::GetUpVector() const
{
	return up;
}

void Camera::UpdateProjectionType()
{
	switch (cameraType)
	{
	case ECameraType::Orthographic:
		SetOrthographicCamera();
		break;

	default:
		SetPerspectiveCamera();
		break;
	}

	GameEngine->GetRenderingSystem()->UpdateProjection(projection);
}

void Camera::Tick(float deltaTime)
{

}

void Camera::SetPerspectiveCamera()
{
	const float aspectRatio = GameEngine->GetRenderingSystem()->GetAspectRatio();
	projection = glm::perspective(glm::radians(fieldOfView), aspectRatio, nearView, farView);
	projection[1][1] *= -1;
}

void Camera::SetOrthographicCamera()
{
	float aspectRatio = GameEngine->GetRenderingSystem()->GetAspectRatio();

	float left = -ortographicSize * aspectRatio;
	float right = ortographicSize * aspectRatio;

	projection = glm::ortho(left, right, -ortographicSize, ortographicSize, -cameraZ, cameraZ);
	projection[1][1] *= -1; // Flip Y because glm uses Open GL coordinates
}

void Camera::UpdateVectors()
{
	if (cameraType == ECameraType::Perspective)
	{
		view = glm::lookAt(
			position,
			position + forward,
			up);
	}
	else
	{
		view = glm::translate(glm::mat4(1.0f), position);
	}
}
