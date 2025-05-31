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

	const DescriptorSetLayoutInfo& GetGlobalLayout() const { return cameraMatricesLayout; }
	const DescriptorSetLayoutInfo& GetLightLayout() const { return lightLayout; }
	const DescriptorSetLayoutInfo& GetAnimationLayout() const { return animationLayout; }

	AllocatedBuffer GetMatriceBuffer() const { return matriceBuffer; }
	AllocatedBuffer GetLightBuffer() const { return lightBuffer; }
	AllocatedBuffer GetAnimationBuffer() const { return animationBuffer; }
	
	GenericHandle GetGlobalDescriptorSet() const { return cameraMatricesDescriptorSet; }
	GenericHandle GetLightDescriptorSet() const { return lightDescriptorSet; }	
	GenericHandle GetAnimationDescriptorSet() const { return animationDescriptorSet; }

private:
	RenderingInterface* renderingInterface = nullptr;

	GenericHandle cameraMatricesDescriptorSet;
	GenericHandle lightDescriptorSet;	
	GenericHandle animationDescriptorSet;

	AllocatedBuffer lightBuffer;
	AllocatedBuffer matriceBuffer;
	AllocatedBuffer animationBuffer;

	DescriptorSetLayoutInfo cameraMatricesLayout;
	DescriptorSetLayoutInfo animationLayout;
	DescriptorSetLayoutInfo lightLayout;
};