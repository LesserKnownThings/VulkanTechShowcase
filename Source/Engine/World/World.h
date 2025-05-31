#pragma once

#include <cstdint>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

class AnimationSystem;
class CameraSystem;
class LightSystem;

struct Camera;

class World
{
public:
	virtual void Initialize();
	virtual void UnInitialize();

protected:
	virtual void Draw();
	virtual void Tick(float deltaTime);

	Camera* mainCam = nullptr;

	AnimationSystem* animationSystem = nullptr;
	CameraSystem* cameraSystem = nullptr;
	LightSystem* lightSystem = nullptr;

	entt::registry registry;

private:
	void CreateCharacter();

	void HandleCameraMovement(float deltaTime);
	void HandleCameraLook(float deltaTime);

	void HandleMouseButton(uint8_t button, bool isPressed, const glm::vec2& position);

	bool isLooking = false;
};

