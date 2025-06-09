#include "AnimationUtilities.h"
#include "AssetManager/Animation/BoneData.h"

#include <cstdint>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

float GetScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
{
	float framesDiff = nextTimeStamp - lastTimeStamp;
	if (framesDiff <= 0.0f)
		return 0.0f;

	float midWayLength = animationTime - lastTimeStamp;
	float scaleFactor = midWayLength / framesDiff;

	if (scaleFactor < 0.0f) scaleFactor = 0.0f;
	else if (scaleFactor > 1.0f) scaleFactor = 1.0f;

	return scaleFactor;
}

glm::mat4 InterpolatePosition(const BoneInstance& boneInstance, float animationTime)
{
	if (boneInstance.numPositions == 1)
	{
		return glm::translate(glm::mat4(1.0f), boneInstance.positions[0].position);
	}

	int32_t p0Index = 0;
	for (int32_t i = 0; i < boneInstance.numPositions - 1; ++i)
	{
		if (animationTime < boneInstance.positions[i + 1].timeStamp)
		{
			p0Index = i;
			break;
		}
	}
	int32_t p1Index = p0Index + 1;

	float scaleFactor = GetScaleFactor(boneInstance.positions[p0Index].timeStamp,
		boneInstance.positions[p1Index].timeStamp, animationTime);
	glm::vec3 finalPosition = glm::mix(boneInstance.positions[p0Index].position,
		boneInstance.positions[p1Index].position, scaleFactor);

	return glm::translate(glm::mat4(1.0f), finalPosition);
}

glm::mat4 InterpolateRotation(const BoneInstance& boneInstance, float animationTime)
{
	if (boneInstance.numRotations == 1)
	{
		auto rotation = glm::normalize(boneInstance.rotations[0].rotation);
		return glm::toMat4(rotation);
	}

	int32_t p0Index = 0;
	for (int32_t i = 0; i < boneInstance.numRotations - 1; ++i)
	{
		if (animationTime < boneInstance.rotations[i + 1].timeStamp)
		{
			p0Index = i;
			break;
		}
	}
	int32_t p1Index = p0Index + 1;
	float scaleFactor = GetScaleFactor(boneInstance.rotations[p0Index].timeStamp,
		boneInstance.rotations[p1Index].timeStamp, animationTime);
	glm::quat finalRotation = glm::slerp(boneInstance.rotations[p0Index].rotation,
		boneInstance.rotations[p1Index].rotation, scaleFactor);
	finalRotation = glm::normalize(finalRotation);
	return glm::toMat4(finalRotation);
}

glm::mat4 InterpolateScale(const BoneInstance& boneInstance, float animationTime)
{
	if (boneInstance.numScales == 1)
	{
		auto scale = boneInstance.scales[0].scale;
		return glm::scale(glm::mat4(1.0f), scale);
	}

	int32_t p0Index = 0;
	for (int32_t i = 0; i < boneInstance.numScales - 1; ++i)
	{
		if (animationTime < boneInstance.scales[i + 1].timeStamp)
		{
			p0Index = i;
			break;
		}
	}
	int32_t p1Index = p0Index + 1;

	float scaleFactor = GetScaleFactor(boneInstance.scales[p0Index].timeStamp,
		boneInstance.scales[p1Index].timeStamp, animationTime);

	glm::vec3 finalScale = glm::mix(boneInstance.scales[p0Index].scale, boneInstance.scales[p1Index].scale, scaleFactor);
	return glm::scale(glm::mat4(1.0f), finalScale);
}

glm::mat4 AnimationUtilities::InterpolateBone(const BoneInstance& boneInstance, float animationTime)
{
	glm::mat4 translation = InterpolatePosition(boneInstance, animationTime);
	glm::mat4 rotation = InterpolateRotation(boneInstance, animationTime);
	glm::mat4 scale = InterpolateScale(boneInstance, animationTime);
	return translation * rotation * scale;
}
