#pragma once

#include "AnimationData.h"
#include "BoneData.h"
#include "Utilities/AlignedVectors.h"

#include <array>
#include <glm/glm.hpp>
#include <vector>

class Skeleton;

class Animator
{
public:
	void Initialize();

	void Run(float deltaTime);

	void SetSkeleton(Skeleton* inSkeleton) { skeleton = inSkeleton; }
	void AddAnimation(const AnimationInstance& animationInstance);

	const std::array<glm::mat4, MAX_BONES>& GetBoneTransforms() const { return boneTransforms; }

private:
	void CalculateBoneTransform(const BoneNode& node, glm::mat4 parentTransform);

	float currentTime = 0.0f;

	uint32_t currentAnimationIndex = 0;
	std::vector<AnimationInstance> animations{};

	Skeleton* skeleton = nullptr;
	std::array<glm::mat4, MAX_BONES> boneTransforms;
};