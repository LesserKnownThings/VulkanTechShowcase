#pragma once

#include "Rendering/Vulkan/RenderPipeline.h"

class LitPipeline : public RenderPipeline
{
public:
	void DestroyPipeline() override;

protected:
	void CreateVertexInputInfo(VkPipelineVertexInputStateCreateInfo& vertexInputInfo) override;
	void CreatePipelineDescriptorLayoutSets(std::vector<VkDescriptorSetLayout>& outDescriptorSetLayouts) override;
	std::string GetShaderPath() const override { return "Data/Engine/Shaders/Lit"; }

private:
	VkDescriptorSetLayout samplerLayout = VK_NULL_HANDLE;
};