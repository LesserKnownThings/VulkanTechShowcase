#pragma once

#include "DescriptorInfo.h"
#include "Rendering/AbstractData.h"

#include <array>

class RenderingInterface;

class DescriptorRegistry
{
public:
	DescriptorRegistry(RenderingInterface* inRenderingInterface);

	void Initialize();
	void UnInitialize();

	const DescriptorSetLayoutInfo& GetCameraMatricesLayout() const { return cameraMatricesLayout; }
	const DescriptorSetLayoutInfo& GetLightLayout() const { return lightLayout; }
	const DescriptorSetLayoutInfo& GetAnimationLayout() const { return animationLayout; }
	const DescriptorSetLayoutInfo& GetShadowLayout() const { return shadowLayout; }

	AllocatedBuffer GetMatriceBuffer() const { return matriceBuffer; }
	AllocatedBuffer GetLightBuffer() const { return lightBuffer; }
	AllocatedBuffer GetAnimationBuffer() const { return animationBuffer; }
	AllocatedBuffer GetLightSMBuffer() const { return lightSMBuffer; }
	AllocatedBuffer GetShadowDataBuffer() const { return shadowDataBuffer; }

	GenericHandle GetCameraMatricesDescriptorSet() const { return cameraMatricesDescriptorSet; }
	GenericHandle GetLightDescriptorSet() const { return lightDescriptorSet; }
	GenericHandle GetAnimationDescriptorSet() const { return animationDescriptorSet; }
	GenericHandle GetShadowDescriptorSet(uint32_t index) const { return shadowDescriptorSets[index]; }

private:
	RenderingInterface* renderingInterface = nullptr;

	GenericHandle cameraMatricesDescriptorSet;
	GenericHandle lightDescriptorSet;
	GenericHandle animationDescriptorSet;

	AllocatedBuffer lightBuffer;
	AllocatedBuffer matriceBuffer;
	AllocatedBuffer animationBuffer;

	AllocatedBuffer lightSMBuffer;
	AllocatedBuffer shadowDataBuffer;
	std::array<GenericHandle, MAX_FRAMES_IN_FLIGHT> shadowDescriptorSets;

	DescriptorSetLayoutInfo cameraMatricesLayout;
	DescriptorSetLayoutInfo animationLayout;
	DescriptorSetLayoutInfo lightLayout;
	DescriptorSetLayoutInfo shadowLayout;
};