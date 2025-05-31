#include "Animator.h"
#include "AnimationUtilities.h"
#include "Skeleton.h"

#include <iostream>

void Animator::Initialize()
{

}

void Animator::Run(float deltaTime)
{
	if (animations.size() > 0)
	{
		const AnimationInstance& instance = animations[currentAnimationIndex];

		currentTime += instance.animationData.ticksPerSecond * deltaTime;
		if (currentTime > instance.animationData.duration)
		{
			currentTime = 0.0f;
			currentAnimationIndex++;
			currentAnimationIndex = fmod(currentAnimationIndex, animations.size());
		}

		const SkeletonData& skeletonData = instance.skeleton->GetSkeletonData();
		uint32_t count = 0;
		CalculateBoneTransform(skeletonData.rootBone, glm::mat4(1.0f), count);
		std::cout << count << std::endl;
	}
}

void Animator::CalculateBoneTransform(const BoneNode& node, glm::mat4 parentTransform, uint32_t& count)
{
	const AnimationInstance& instance = animations[currentAnimationIndex];

	glm::mat4 nodeTransform = node.transform;

	auto it = instance.animationData.boneInstanceMap.find(node.name);
	if (it != instance.animationData.boneInstanceMap.end())
	{
		const BoneInstance& bone = it->second;
		nodeTransform = AnimationUtilities::InterpolateBone(bone, currentTime);
	}

	glm::mat4 globalTransform = parentTransform * nodeTransform;

	const SkeletonData& skeletonData = instance.skeleton->GetSkeletonData();
	const auto& boneInfoMap = skeletonData.boneInfoMap;

	auto boneInfoIT = boneInfoMap.find(node.name);
	if (boneInfoIT != boneInfoMap.end())
	{
		count++;
		int32_t index = boneInfoIT->second.id;
		glm::mat4 offset = boneInfoIT->second.offset;
		boneTransforms[index] = globalTransform * offset;
		boneNormals[index] = AlignedMatrix3{ glm::transpose(glm::inverse(glm::mat3(boneTransforms[index]))), glm::vec3(0.0f) };
	}

	for (int32_t i = 0; i < node.childrenCount; ++i)
	{
		CalculateBoneTransform(node.children[i], globalTransform, count);
	}
}

void Animator::AddAnimation(const AnimationInstance& animationInstance)
{
	animations.push_back(animationInstance);
}
