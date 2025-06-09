#pragma once

#include "Utilities/Delegate.h"

#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <vector>

enum ECameraType : uint8_t
{
	Orthographic,
	Perspective
};

constexpr glm::vec3 WORLD_RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);
constexpr glm::vec3 WORLD_UP = glm::vec3(0.0f, 1.0f, 0.0f);
constexpr glm::vec3 WORLD_FORWARD = glm::vec3(0.0f, 0.0f, 1.0f);

struct CameraData
{
	ECameraType cameraType = Perspective;

	//***Perspective settings
	float fieldOfView = 60.0f;
	float nearView = 1.0f;
	float farView = 1000.0f;

	//***Orthographic settings
	float ortographicSize = 10.0f;
	float cameraZ = 1000.0f;

	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 eulers = glm::vec3(0.0f);

	glm::vec3 forward = WORLD_FORWARD;
	glm::vec3 up = WORLD_UP;
	glm::vec3 right = WORLD_RIGHT;

	glm::mat4 projection = glm::mat4(1.0f);
	glm::mat4 view = glm::mat4(1.0f);
};

struct Camera
{
public:
	~Camera();
	Camera();
	Camera(const CameraData& inData);

	void SetPosition(const glm::vec3& newPosition);
	void SetRotation(const glm::vec3& eulerAngles);
	void Rotate(const glm::vec3& axis, const glm::vec2& pitchClamp = glm::vec2(-89.0f, 89.0f));

	static std::vector<glm::vec4> GetWorldSpaceFrustumCorners(const glm::mat4& projection, const glm::mat4& view);
	static glm::vec3 GetWorldSpaceFrustumCenter(const std::vector<glm::vec4>& corners);

	CameraData data;

	bool projectionChanged = false;
	bool viewChanged = false;
	std::vector<float> shadowCascadeLevels;

private:
	void Initialize();
	void UpdateVectors();
	void UpdateDirections();
	void UpdateProjectionType();
	void HandleWindowResize(float width, float height);

	void SetPerspectiveCamera();
	void SetOrthographicCamera();
};
