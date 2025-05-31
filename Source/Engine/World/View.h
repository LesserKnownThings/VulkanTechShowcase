#pragma once

#include "Camera/Camera.h"
#include <entt/entity/registry.hpp>

class AnimationSystem;
class LightSystem;

struct View
{
	const Camera& camera;
	const entt::registry& registry;
	const std::vector<entt::entity>& entitiesInView;
	const std::vector<entt::entity>& lightsInView;

	AnimationSystem* animationSystem = nullptr;
	LightSystem* lightSystem = nullptr;
};