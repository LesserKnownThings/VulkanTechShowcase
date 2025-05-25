#pragma once

#include <entt/entt.hpp>
#include <unordered_map>
#include <vector>

class Camera;

class World
{
public:
	virtual void Initialize();
	virtual void UnInitialize();

protected:
	virtual void Draw();
	virtual void Tick(float deltaTime);

	Camera* camera = nullptr;

	entt::registry registry;
};

