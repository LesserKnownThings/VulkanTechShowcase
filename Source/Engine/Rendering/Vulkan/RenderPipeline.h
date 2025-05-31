#pragma once

#include "Rendering/Descriptors/DescriptorInfo.h"
#include "Rendering/Material/Material.h"
#include "VkContext.h"

#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <volk.h>

class IDescriptorDataProvider;
class RenderingInterface;
class Texture;

struct DescriptorSetBlock;

class RenderPipeline
{
public:
	void Initialize(const VkContext& inContext, RenderingInterface* inRenderingInterface);

	virtual void DestroyPipeline();

	void Bind(VkCommandBuffer cmdBuffer);

	// TODO specialized descriptor sets (like compute shader buffers)

	virtual void AllocateMaterialDescriptorSet(GenericHandle& outDescriptorSet);
	virtual void UpdateMaterialDescriptorSet(const std::unordered_map<EngineName, DescriptorDataProvider>& dataProviders);

	virtual EPipelineType GetType() const = 0;

	VkPipeline GetPipeline() const { return graphicsPipeline; }
	VkPipelineLayout GetLayout() const { return pipelineLayout; }
	const std::string& GetName() const { return name; }

	bool SupportsCamera() const;
	bool SupportsLight() const;
	bool SupportsAnimations() const;

	const DescriptorSetLayoutInfo& GetMaterialLayout() const { return materialLayout; }

protected:
	virtual bool CreateVertexSpecializationInfo(VkSpecializationInfo& outInfo) = 0;
	virtual std::vector<VkSpecializationMapEntry> CreateVertexSpecializationMap(size_t& dataSize) = 0;
	virtual bool CreateFragmentSpecializationInfo(VkSpecializationInfo& outInfo) = 0;
	virtual std::vector<VkSpecializationMapEntry> CreateFragmentSpecializationMap(size_t& dataSize) = 0;

	virtual void CreateVertexInputInfo(std::vector<VkVertexInputBindingDescription>& bindingDescriptions, std::vector<VkVertexInputAttributeDescription>& attributeDescriptions) = 0;
	virtual void CreatePipelineDescriptorLayoutSets(std::vector<VkDescriptorSetLayout>& outDescriptorSetLayouts) = 0;
	virtual std::vector<VkPushConstantRange> GetPipelinePushConstants() = 0;

	virtual std::string GetShaderPath() const = 0;

	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	// Common Settings
	std::string name;

	VkContext context;
	RenderingInterface* renderingInterface = nullptr;

	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	VkPipeline graphicsPipeline = VK_NULL_HANDLE;

	DescriptorSetLayoutInfo materialLayout;
	std::vector<DescriptorBindingInfo> descriptorBindings;

	uint32_t flags = 0;

	std::vector<VkDynamicState> dynamicStates =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
};