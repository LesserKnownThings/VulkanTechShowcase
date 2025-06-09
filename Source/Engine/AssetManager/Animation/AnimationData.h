#pragma once

#include "BoneData.h"
#include "EngineName.h"
#include "Utilities/AlignedVectors.h"

#include <array>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

struct AnimationLayout
{
	alignas(16) std::array<glm::mat4, MAX_BONES> transforms;
};

struct AnimationData
{
	int32_t ticksPerSecond;
	float duration;

	std::unordered_map<EngineName, BoneInstance> boneInstanceMap;
};

class Skeleton;

struct AnimationInstance
{
	AnimationData animationData;
	Skeleton* skeleton = nullptr;
};
