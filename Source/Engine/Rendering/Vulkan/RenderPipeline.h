#pragma once

#include "Rendering/Descriptors/DescriptorInfo.h"
#include "VkContext.h"

#include <unordered_map>
#include <vector>
#include <volk.h>

class RenderingInterface;

class RenderPipeline
{
public:
	RenderPipeline(const VkContext& inContext, RenderingInterface* inRenderingInterface);

	virtual void DestroyPipeline();

	void Bind(VkCommandBuffer cmdBuffer);

	virtual void UpdateDescriptorSet(const std::unordered_map<EngineName, DescriptorDataProvider>& dataProviders);

	virtual EPipelineType GetType() const = 0;

	VkPipelineLayout GetLayout() const { return pipelineLayout; }

	bool TryGetDescriptorLayoutForOwner(EDescriptorOwner owner, DescriptorSetLayoutInfo& outLayoutInfo) const;

	bool SupportsCamera() const;
	bool SupportsLight() const;
	bool SupportsAnimation() const;

protected:
	VkShaderModule CreateShaderModule(const std::vector<char>& code);

	RenderingInterface* renderingInterface = nullptr;
	const VkContext& context;

	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
	VkPipeline pipeline = VK_NULL_HANDLE;

	uint32_t flags = 0;

	/// <summary>
	/// Specific descriptor bindings for pipelines, these can used to set specific elements in the 
	/// descriptor set using semantics.
	/// </summary>
	std::vector<DescriptorBindingInfo> descriptorBindings;
	std::vector<DescriptorSetLayoutInfo> descriptorLayouts;	
};