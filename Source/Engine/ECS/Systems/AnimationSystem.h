#pragma once

#include "SystemBase.h"
// TODO move the asset part in the asset manager

#include <string>
#include <unordered_map>
#include <vector>

class Animator;

struct AnimatorComponent;

class AnimationSystem : public SystemBase
{
public:
	//I know I'm not following the rule of 5 here, but this works more like a helper class than an object
	~AnimationSystem();

	AnimatorComponent CreateAnimator(uint32_t skeletonHandle, const std::vector<std::string>& animations);

	void Run(uint32_t handle, float deltaTime);
	void GatherDrawData(uint32_t handle, uint32_t entityIndex, uint32_t frameIndex);

private:
	std::unordered_map<uint32_t, Animator*> animators;

	// The animation buffer will update each frame, so there's no need to allocated it each time
	void* buffer = nullptr;
};