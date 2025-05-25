#pragma once

#include "Rendering/AbstractData.h"

#include <vma/vk_mem_alloc.h>
#include <volk.h>

class RenderUtilities
{
public:
	static VkBuffer GenericHandleToBuffer(GenericHandle handle)
	{
		return reinterpret_cast<VkBuffer>(std::get<uintptr_t>(handle));
	}

	static VmaAllocation GenericHandleToAllocation(GenericHandle handle)
	{
		return reinterpret_cast<VmaAllocation>(std::get<uintptr_t>(handle));
	}

	static VkImage GenericHandleToImage(GenericHandle handle)
	{
		return reinterpret_cast<VkImage>(std::get<uintptr_t>(handle));
	}

	static VkImageView GenericHandleToImageView(GenericHandle handle)
	{
		return reinterpret_cast<VkImageView>(std::get<uintptr_t>(handle));
	}

	static VkSampler GenericHandleToImageSampler(GenericHandle handle)
	{
		return reinterpret_cast<VkSampler>(std::get<uintptr_t>(handle));
	}

	static VkDescriptorSet GenericHandleToDescriptorSet(GenericHandle handle)
	{
		return reinterpret_cast<VkDescriptorSet>(std::get<uintptr_t>(handle));
	}

	static VkDescriptorSetLayout GenericHandleToDescriptorSetLayout(GenericHandle handle)
	{
		return reinterpret_cast<VkDescriptorSetLayout>(std::get<uintptr_t>(handle));
	}

	static GenericHandle BufferToGenericHandle(VkBuffer buffer)
	{
		return reinterpret_cast<uintptr_t>(buffer);
	}

	static GenericHandle AllocationToGenericHandle(VmaAllocation allocation)
	{
		return reinterpret_cast<uintptr_t>(allocation);
	}

	static GenericHandle ImageToGenericHandle(VkImage image)
	{
		return reinterpret_cast<uintptr_t>(image);
	}

	static GenericHandle ImageViewToGenericHandle(VkImageView imageView)
	{
		return reinterpret_cast<uintptr_t>(imageView);
	}

	static GenericHandle ImageSamplerToGenericHandle(VkSampler sampler)
	{
		return reinterpret_cast<uintptr_t>(sampler);
	}

	static GenericHandle DescriptorSetToGenericHandle(VkDescriptorSet descriptorSet)
	{
		return reinterpret_cast<uintptr_t>(descriptorSet);
	}

	static GenericHandle DescriptorSetLayoutToGenericHandle(VkDescriptorSetLayout descriptorSetLayout)
	{
		return reinterpret_cast<uintptr_t>(descriptorSetLayout);
	}
};