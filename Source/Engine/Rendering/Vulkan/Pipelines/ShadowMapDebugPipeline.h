#pragma once

#include "Rendering/Descriptors/DescriptorInfo.h"
#include "Rendering/Vulkan/RenderPipeline.h"

class ShadowMapDebugPipeline : public RenderPipeline
{
public:
	ShadowMapDebugPipeline(const VkContext& inContext, RenderingInterface* inRenderingInterface);
	void DestroyPipeline() override;

	EPipelineType GetType() const override;

	GenericHandle GetDebugDescriptorSet() const { return debugSet; }
	AllocatedBuffer GetDebugBuffer() const { return debugBuffer; }

private:
	void CreatePipelineDescriptorSets(std::array<VkDescriptorSetLayout, 2>& layouts);

	const std::string shaderPath = "Data/Engine/Shaders/DebugShadowMapping";

	AllocatedBuffer debugBuffer;
	DescriptorSetLayoutInfo debugLayout;
	GenericHandle debugSet;

	std::vector<VkDynamicState> dynamicStates =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
};