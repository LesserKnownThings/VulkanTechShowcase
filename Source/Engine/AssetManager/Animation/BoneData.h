#pragma once

#include "EngineName.h"

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <unordered_map>
#include <vector>

constexpr int32_t MAX_BONE_INFLUENCE = 4;
constexpr int32_t MAX_BONES = 100;
constexpr uint32_t MAX_ANIMATED_ENTITIES = 100;

struct KeyPosition
{
	glm::vec3 position;
	float timeStamp;
};

struct KeyRotation
{
	glm::quat rotation;
	float timeStamp;
};

struct KeyScale
{
	glm::vec3 scale;
	float timeStamp;
};

struct BoneInstance
{
	std::vector<KeyPosition> positions;
	std::vector<KeyRotation> rotations;
	std::vector<KeyScale> scales;

	int32_t numPositions;
	int32_t numRotations;
	int32_t numScales;
};

struct BoneInfo
{
	int32_t id;
	glm::mat4 offset;
};

struct BoneNode
{
	EngineName name;
	glm::mat4 transform;
	int32_t childrenCount;
	std::vector<BoneNode> children;
};

struct SkeletonData
{
	int32_t boneInfoCount;

	BoneNode rootBone;
	std::unordered_map<EngineName, BoneInfo> boneInfoMap;	
};
