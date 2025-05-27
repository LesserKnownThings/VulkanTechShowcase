#pragma once

#include <cstdint>
#include <entt/entt.hpp>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

class Camera;
class LightSystem;

class World
{
public:
	virtual void Initialize();
	virtual void UnInitialize();

protected:
	virtual void Draw();
	virtual void Tick(float deltaTime);

	Camera* camera = nullptr;
	LightSystem* lightSystem = nullptr;

	entt::registry registry;

private:
	void HandleCameraMovement(float deltaTime);
	void HandleCameraLook(float deltaTime);

	void HandleMouseButton(uint8_t button, bool isPressed, const glm::vec2& position);

	bool isLooking = false;
};

