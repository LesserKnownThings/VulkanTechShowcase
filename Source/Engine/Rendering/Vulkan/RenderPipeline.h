#pragma once

#include "EngineName.h"
#include "VkContext.h"

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <volk.h>

class RenderPipeline
{
public:
	virtual void Initialize(const VkContext& inContext);
	virtual void DestroyPipeline();

	VkPipeline GetPipeline() const { return graphicsPipeline; }
	VkPipelineLayout GetLayout() const { return pipelineLayout; }
	const std::string& GetName() const { return name; }
	uint32_t GetDescriptorSetCount() const { return descriptorSetCount; }

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

	uint32_t descriptorSetCount = 0;

	std::vector<VkDynamicState> dynamicStates =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
};