#pragma once

#include "AnimationData.h"
#include "BoneData.h"
#include "Utilities/AlignedVectors.h"

#include <array>
#include <glm/glm.hpp>
#include <vector>

class Animator
{
public:
	void Initialize();

	void Run(float deltaTime);

	void AddAnimation(const AnimationInstance& animationInstance);

	const std::array<glm::mat4, MAX_BONES>& GetBoneTransforms() const { return boneTransforms; }
	const std::array<AlignedMatrix3, MAX_BONES>& GetBoneNormals() const { return boneNormals; }

private:
	void CalculateBoneTransform(const BoneNode& node, glm::mat4 parentTransform, uint32_t& count);

	float currentTime = 0.0f;

	uint32_t currentAnimationIndex = 0;
	std::vector<AnimationInstance> animations{};

	std::array<glm::mat4, MAX_BONES> boneTransforms;
	std::array<AlignedMatrix3, MAX_BONES> boneNormals;
};