#include "AnimationSystem.h"
#include "AssetManager/AssetManager.h"
#include "AssetManager/Animation/AnimationData.h"
#include "AssetManager/Animation/Animator.h"
#include "AssetManager/Animation/BoneData.h"
#include "AssetManager/Animation/Skeleton.h"
#include "ECS/Components/Components.h"
#include "Engine.h"
#include "Rendering/Descriptors/DescriptorRegistry.h"
#include "Rendering/RenderingInterface.h"
#include "Utilities/MeshImporter.h"

AnimationSystem::~AnimationSystem()
{
	free(buffer);
}

AnimatorComponent AnimationSystem::CreateAnimator(uint32_t skeletonHandle, const std::vector<std::string>& animations)
{
	uint32_t handle = GenerateHandle();

	AssetManager& assetManager = AssetManager::Get();

	Skeleton* skeleton = assetManager.LoadAsset<Skeleton>(skeletonHandle);
	const SkeletonData& skeletonData = skeleton->GetSkeletonData();

	Animator* animator = new Animator();
	animator->Initialize();
	animator->SetSkeleton(skeleton);

	for (const std::string& animation : animations)
	{
		AnimationInstance instance{};
		instance.skeleton = skeleton;
		MeshImporter::ImportAnimation(animation, skeletonData, instance.animationData);

		animator->AddAnimation(instance);
	}

	animators[handle] = animator;

	return AnimatorComponent{ handle };
}

void AnimationSystem::Run(uint32_t handle, float deltaTime)
{
	if (buffer == nullptr)
	{
		buffer = malloc(sizeof(glm::mat4) * MAX_BONES * 2 + sizeof(uint32_t));
	}

	auto it = animators.find(handle);
	if (it != animators.end())
	{
		Animator* animator = it->second;
		animator->Run(deltaTime);
	}
}

void AnimationSystem::GatherDrawData(uint32_t handle, uint32_t entityIndex, uint32_t frameIndex)
{
	auto it = animators.find(handle);
	if (it != animators.end())
	{
		Animator* animator = it->second;

		const uint32_t singleBufferSize = sizeof(AnimationLayout);

		const std::array<glm::mat4, MAX_BONES>& transforms = animator->GetBoneTransforms();

		AnimationLayout layout
		{
			transforms,
		};

		memcpy(buffer, &layout, singleBufferSize);

		RenderingInterface* renderingInterface = GameEngine->GetRenderingSystem();
		DescriptorRegistry* descriptorRegistry = renderingInterface->GetDescriptorRegistry();

		AllocatedBuffer animationBuffer = descriptorRegistry->GetAnimationBuffer();
		uint32_t offset = frameIndex * sizeof(AnimationLayout) * MAX_ANIMATED_ENTITIES;
		uint32_t entityOffset = sizeof(AnimationLayout) * entityIndex;
		renderingInterface->UpdateBuffer(animationBuffer, offset + entityOffset, singleBufferSize, buffer);
	}
}
