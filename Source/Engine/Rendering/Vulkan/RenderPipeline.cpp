#include "RenderPipeline.h"
#include "Rendering/Vulkan/RenderUtilities.h"

#include <iostream>

RenderPipeline::RenderPipeline(const VkContext& inContext, RenderingInterface* inRenderingInterface)
	: context(inContext), renderingInterface(inRenderingInterface)
{
}

void RenderPipeline::DestroyPipeline()
{
	for (DescriptorSetLayoutInfo& layoutInfo : descriptorLayouts)
	{
		VkDescriptorSetLayout layout = RenderUtilities::GenericHandleToDescriptorSetLayout(layoutInfo.layout);
		vkDestroyDescriptorSetLayout(context.device, layout, nullptr);
	}

	vkDestroyPipeline(context.device, pipeline, nullptr);
	vkDestroyPipelineLayout(context.device, pipelineLayout, nullptr);
}

void RenderPipeline::Bind(VkCommandBuffer cmdBuffer)
{
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}


VkShaderModule RenderPipeline::CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	createInfo.pNext = nullptr;

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(context.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		std::cerr << "Failed to create shader module!" << std::endl;
	}

	return shaderModule;
}

void RenderPipeline::UpdateDescriptorSet(const std::unordered_map<EngineName, DescriptorDataProvider>& dataProviders)
{
	std::vector<VkWriteDescriptorSet> writes;

	for (const DescriptorBindingInfo& layoutInfo : descriptorBindings)
	{
		auto it = dataProviders.find(layoutInfo.semantic);
		if (it != dataProviders.end())
		{
			const DescriptorDataProvider& provider = it->second;

			VkDescriptorImageInfo imageInfo{};
			if (provider.providerType == EDescriptorDataProviderType::Texture || provider.providerType == EDescriptorDataProviderType::All)
			{
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = RenderUtilities::GenericHandleToImageView(provider.texture.view);
				imageInfo.sampler = RenderUtilities::GenericHandleToImageSampler(provider.texture.sampler);
			}

			VkDescriptorBufferInfo bufferInfo{};
			if (provider.providerType == EDescriptorDataProviderType::Buffer || provider.providerType == EDescriptorDataProviderType::All)
			{
				bufferInfo.buffer = RenderUtilities::GenericHandleToBuffer(provider.dataProviderBuffer.buffer.buffer);
				bufferInfo.offset = provider.dataProviderBuffer.offset;
				bufferInfo.range = provider.dataProviderBuffer.range;
			}

			VkWriteDescriptorSet write{};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = RenderUtilities::GenericHandleToDescriptorSet(provider.descriptorSet);
			write.dstBinding = layoutInfo.binding;
			write.dstArrayElement = 0;
			write.descriptorType = static_cast<VkDescriptorType>(layoutInfo.type);
			write.descriptorCount = 1;
			write.pImageInfo = &imageInfo;
			write.pBufferInfo = &bufferInfo;
			writes.push_back(write);
		}
	}

	vkUpdateDescriptorSets(context.device, writes.size(), writes.data(), 0, nullptr);
}

bool RenderPipeline::TryGetDescriptorLayoutForOwner(EDescriptorOwner owner, DescriptorSetLayoutInfo& outLayoutInfo) const
{
	for (const DescriptorSetLayoutInfo& descriptorLayout : descriptorLayouts)
	{
		if (descriptorLayout.owner == owner)
		{
			outLayoutInfo = descriptorLayout;
			return true;
		}
	}
	return false;
}

bool RenderPipeline::SupportsCamera() const
{
	return (flags & MATRICES_DESCRIPTOR_FLAG) != 0;
}

bool RenderPipeline::SupportsLight() const
{
	return (flags & LIGHT_DESCRIPTOR_FLAG) != 0;
}

bool RenderPipeline::SupportsAnimation() const
{
	return (flags & ANIMATION_DESCRIPTOR_FLAG) != 0;
}
