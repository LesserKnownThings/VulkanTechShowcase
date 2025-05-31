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
	alignas(16) std::array<AlignedMatrix3, MAX_BONES> normals;
};

struct AnimationNodeData
{
	glm::mat4 transform;
	EngineName name;
	int32_t childrenCount;
	std::vector<AnimationNodeData> children;
};

struct AnimationData
{
	int32_t ticksPerSecond;
	float duration;

	AnimationNodeData root;
	std::unordered_map<EngineName, BoneInstance> boneInstanceMap;
};

class Skeleton;

struct AnimationInstance
{
	AnimationData animationData;
	Skeleton* skeleton = nullptr;
};
