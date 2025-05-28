#pragma once

class Camera;

#include <entt/entity/registry.hpp>

struct View
{
	Camera* camera = nullptr;
	const entt::registry& registry;
	const std::vector<entt::entity>& entitiesInView;
	const std::vector<entt::entity>& lightsInView;
};