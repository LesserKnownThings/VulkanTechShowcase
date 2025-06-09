#pragma once

#include "Camera/Camera.h"

#include <cstdint>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

class AnimationSystem;
class CameraSystem;
class LightSystem;

struct View;

class World
{
public:
	virtual void Initialize();
	virtual void UnInitialize();

	void GetWorldView(View* view);

protected:
	virtual void Tick(float deltaTime);

	Camera mainCam;

	AnimationSystem* animationSystem = nullptr;
	CameraSystem* cameraSystem = nullptr;
	LightSystem* lightSystem = nullptr;

	entt::registry registry;

private:
	void CreateCharacter(const glm::vec3& pos);
	void CreateFloor();

	void HandleCameraMovement(float deltaTime);
	void HandleCameraLook(float deltaTime);

	void HandleMouseButton(uint8_t button, bool isPressed, const glm::vec2& position);

	bool isLooking = false;
};

