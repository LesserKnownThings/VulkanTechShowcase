#pragma once

#include <glm/glm.hpp>

struct BoneInstance;

class AnimationUtilities
{
public:
	static glm::mat4 InterpolateBone(const BoneInstance& boneInstance, float animationTime);
};