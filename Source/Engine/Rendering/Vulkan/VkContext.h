#pragma once

#include <volk.h>
#include <vma/vk_mem_alloc.h>

struct VkContext
{
	VkInstance instance = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkQueue graphicsQueue = VK_NULL_HANDLE;
	VkQueue presentQueue = VK_NULL_HANDLE;
	VkQueue transferQueue = VK_NULL_HANDLE;
	VkCommandPool commandPool = VK_NULL_HANDLE;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	VkRenderPass renderPass = VK_NULL_HANDLE;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

	VmaAllocator allocator = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

	// Global Descriptor Layout Handles
	VkDescriptorSetLayout globalDescriptorSetLayout;
};