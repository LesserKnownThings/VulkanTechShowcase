#pragma once

#include "Camera/Camera.h"
#include <entt/entity/registry.hpp>

class AnimationSystem;
class LightSystem;

struct View
{
	Camera* camera;
	
	std::vector<entt::entity> entitiesInView;
	std::vector<entt::entity> lightsInView;

	entt::registry* registry;

	AnimationSystem* animationSystem = nullptr;
	LightSystem* lightSystem = nullptr;
};