#pragma once

#include "Rendering/Vulkan/RenderPipeline.h"

#include <vector>

class PBRPipeline : public RenderPipeline
{
public:
	void DestroyPipeline() override;

	void AllocateDescriptorSet(GenericHandle& outDescriptorSet) override;
	void UpdateDescriptorSet(GenericHandle descriptorSet, const TextureSetKey& key) override;

protected:
	void CreateVertexInputInfo(VkPipelineVertexInputStateCreateInfo& vertexInputInfo) override;
	void CreatePipelineDescriptorLayoutSets(std::vector<VkDescriptorSetLayout>& outDescriptorSetLayouts) override;
	std::string GetShaderPath() const override { return "Data/Engine/Shaders/Lit"; }

private:
	VkDescriptorSetLayout samplerSetLayout = VK_NULL_HANDLE;
};