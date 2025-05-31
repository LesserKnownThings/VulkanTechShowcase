#include "Animator.h"
#include "AnimationUtilities.h"
#include "Skeleton.h"

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

		CalculateBoneTransform(instance.animationData.root, glm::mat4(1.0f));
	}
}

void Animator::CalculateBoneTransform(const AnimationNodeData& node, glm::mat4 parentTransform)
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
		int32_t index = boneInfoIT->second.id;
		glm::mat4 offset = boneInfoIT->second.offset;
		boneTransforms[index] = globalTransform * offset;
		boneNormals[index] = AlignedMatrix3{ glm::transpose(glm::inverse(glm::mat3(boneTransforms[index]))), glm::vec3(0.0f) };
	}

	for (int32_t i = 0; i < node.childrenCount; ++i)
	{
		CalculateBoneTransform(node.children[i], globalTransform);
	}
}

void Animator::AddAnimation(const AnimationInstance& animationInstance)
{
	animations.push_back(animationInstance);
}
