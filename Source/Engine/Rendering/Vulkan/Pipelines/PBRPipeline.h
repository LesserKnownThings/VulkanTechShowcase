#pragma once

#include "Rendering/Vulkan/RenderPipeline.h"

#include <vector>

class PBRPipeline : public RenderPipeline
{
public:
	PBRPipeline(const VkContext& inContext, RenderingInterface* inRenderingInterface);

	EPipelineType GetType() const override { return EPipelineType::PBR; }

private:
	void CreatePipelineDescriptorLayoutSets(std::array<VkDescriptorSetLayout, 5>& outDescriptorSetLayouts);

	const std::string shaderPath = "Data/Engine/Shaders/PBR";

	std::vector<VkDynamicState> dynamicStates =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
};