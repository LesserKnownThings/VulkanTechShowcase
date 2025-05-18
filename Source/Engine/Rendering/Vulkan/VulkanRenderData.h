#pragma once

#include <volk.h>
#include <vma/vk_mem_alloc.h>

struct VulkanRenderData
{
	VkBuffer vertexBuffer = VK_NULL_HANDLE;
	VmaAllocation vertexMemory = VK_NULL_HANDLE;
	VkBuffer indexBuffer = VK_NULL_HANDLE;
	VmaAllocation indexMemory = VK_NULL_HANDLE;
};