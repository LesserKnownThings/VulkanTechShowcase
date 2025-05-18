#pragma once

#include "ECS/Entity.h"

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

	std::vector<Entity> entities;

	Camera* camera = nullptr;
};