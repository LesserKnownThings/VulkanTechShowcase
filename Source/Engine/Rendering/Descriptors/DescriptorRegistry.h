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

	const DescriptorSetLayoutInfo& GetGlobalLayout() const { return globalLayout; }
	const DescriptorSetLayoutInfo& GetLightLayout() const { return lightLayout; }
	const AllocatedBuffer& GetLightBuffer() const { return lightBuffer; }
	const GenericHandle GetLightDescriptorSet() const { return lightDescriptorSet; }

	const std::array<AllocatedBuffer, MAX_FRAMES_IN_FLIGHT>& GetMatricesBuffers() const { return matricesBuffers; }
	const std::array<GenericHandle, MAX_FRAMES_IN_FLIGHT>& GetGlobalDescriptorSets() const { return globalDescriptorSets; }

private:
	RenderingInterface* renderingInterface = nullptr;

	GenericHandle lightDescriptorSet;
	AllocatedBuffer lightBuffer;

	DescriptorSetLayoutInfo globalLayout;
	DescriptorSetLayoutInfo lightLayout;

	std::array<AllocatedBuffer, MAX_FRAMES_IN_FLIGHT> matricesBuffers;
	std::array<GenericHandle, MAX_FRAMES_IN_FLIGHT> globalDescriptorSets;
};