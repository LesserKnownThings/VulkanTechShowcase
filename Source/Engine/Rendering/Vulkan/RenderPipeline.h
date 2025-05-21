#pragma once

#include "EngineName.h"
#include "Rendering/Material/Material.h"
#include "VkContext.h"

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <volk.h>

class Texture;

class RenderPipeline
{
public:
	virtual void Initialize(const VkContext& inContext);
	virtual void DestroyPipeline();

	virtual void AllocateDescriptorSet(GenericHandle& outDescriptorSet) = 0;
	virtual void UpdateDescriptorSet(GenericHandle descriptorSet, const TextureSetKey& key) = 0;

	VkPipeline GetPipeline() const { return graphicsPipeline; }
	VkPipelineLayout GetLayout() const { return pipelineLayout; }
	const std::string& GetName() const { return name; }

protected:
	virtual void CreateVertexInputInfo(VkPipelineVertexInputStateCreateInfo& vertexInputInfo) = 0;
	virtual void CreatePipelineDescriptorLayoutSets(std::vector<VkDescriptorSetLayout>& outDescriptorSetLayouts) = 0;
	virtual std::string GetShaderPath() const = 0;

	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	// Common Settings
	std::string name;

	VkContext context;

	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	VkPipeline graphicsPipeline = VK_NULL_HANDLE;

	std::vector<VkDynamicState> dynamicStates =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
};