#include "Animator.h"
#include "AnimationUtilities.h"
#include "Skeleton.h"

#include <iostream>

void Animator::Initialize()
{
}

void Animator::Run(float deltaTime)
{
	if (skeleton != nullptr && animations.size() > 0)
	{
		const AnimationInstance& instance = animations[currentAnimationIndex];

		currentTime += instance.animationData.ticksPerSecond * deltaTime;
		if (currentTime > instance.animationData.duration)
		{
			currentTime = 0.0f;
			currentAnimationIndex++;
			currentAnimationIndex = fmod(currentAnimationIndex, animations.size());
		}

		const SkeletonData& skeletonData = skeleton->GetSkeletonData();
		CalculateBoneTransform(skeletonData.rootBone, glm::mat4(1.0f));
	}
}

void Animator::CalculateBoneTransform(const BoneNode& node, glm::mat4 parentTransform)
{
	const AnimationInstance& instance = animations[currentAnimationIndex];
	const SkeletonData& skeletonData = skeleton->GetSkeletonData();
	const auto& boneInfoMap = skeletonData.boneInfoMap;

	glm::mat4 nodeTransform = node.transform;

	auto it = instance.animationData.boneInstanceMap.find(node.name);
	if (it != instance.animationData.boneInstanceMap.end())
	{
		nodeTransform = AnimationUtilities::InterpolateBone(it->second, currentTime);
	}

	glm::mat4 globalTransform = parentTransform * nodeTransform;

	auto boneInfoIT = boneInfoMap.find(node.name);
	if (boneInfoIT != boneInfoMap.end())
	{
		int32_t index = boneInfoIT->second.id;

		glm::mat4 finalTransform = globalTransform * boneInfoIT->second.offset;
		boneTransforms[index] = finalTransform;
	}

	for (const BoneNode& child : node.children)
	{
		CalculateBoneTransform(child, globalTransform);
	}
}

void Animator::AddAnimation(const AnimationInstance& animationInstance)
{
	animations.push_back(animationInstance);
}
