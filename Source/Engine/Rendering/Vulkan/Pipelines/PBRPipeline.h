#pragma once

#include "Rendering/Vulkan/RenderPipeline.h"

#include <vector>

class PBRPipeline : public RenderPipeline
{
public:
	EPipelineType GetType() const override { return EPipelineType::PBR; }

protected:
	void SetSpecializationConstants(VkPipelineShaderStageCreateInfo& vertexShader, VkPipelineShaderStageCreateInfo& fragmentShader) override;
	void CreateVertexInputInfo(VkPipelineVertexInputStateCreateInfo& vertexInputInfo) override;
	void CreatePipelineDescriptorLayoutSets(std::vector<VkDescriptorSetLayout>& outDescriptorSetLayouts) override;
	std::vector<VkPushConstantRange> GetPipelinePushConstants() override;

	std::string GetShaderPath() const override { return "Data/Engine/Shaders/PBR"; }
};