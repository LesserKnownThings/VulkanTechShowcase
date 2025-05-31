#pragma once

#include "Rendering/Vulkan/RenderPipeline.h"

#include <vector>

class PBRPipeline : public RenderPipeline
{
public:
	EPipelineType GetType() const override { return EPipelineType::PBR; }

protected:
	bool CreateVertexSpecializationInfo(VkSpecializationInfo& outInfo) override;
	bool CreateFragmentSpecializationInfo(VkSpecializationInfo& outInfo) override;

	void CreateVertexInputInfo(std::vector<VkVertexInputBindingDescription>& bindingDescriptions, std::vector<VkVertexInputAttributeDescription>& attributeDescriptions) override;
	std::vector<VkSpecializationMapEntry> CreateVertexSpecializationMap(size_t& dataSize) override;
	void CreatePipelineDescriptorLayoutSets(std::vector<VkDescriptorSetLayout>& outDescriptorSetLayouts) override;
	std::vector<VkSpecializationMapEntry> CreateFragmentSpecializationMap(size_t& dataSize) override;

	std::vector<VkPushConstantRange> GetPipelinePushConstants() override;

	std::string GetShaderPath() const override { return "Data/Engine/Shaders/PBR"; }
};