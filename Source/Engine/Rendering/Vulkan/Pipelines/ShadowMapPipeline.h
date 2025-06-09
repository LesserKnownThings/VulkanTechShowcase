#pragma once

#include "Rendering/AbstractData.h"
#include "Rendering/Vulkan/RenderPipeline.h"

class ShadowMapPipeline : public RenderPipeline
{
public:
	ShadowMapPipeline(const VkContext& inContext, RenderingInterface* inRenderingInterface);

	EPipelineType GetType() const override;

private:
	const std::string shaderPath = "Data/Engine/Shaders/ShadowMapping";

	std::vector<VkDynamicState> dynamicStates =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
};