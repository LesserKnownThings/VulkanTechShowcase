#pragma once

#include "EngineName.h"
#include "VkContext.h"

#include <volk.h>

struct VkContext;

struct Frame
{
public:
	Frame() = default;
	Frame(const VkContext& context);
	
	void Destroy(const VkContext& context);

	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

	VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
	VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
	VkFence inFlightFence = VK_NULL_HANDLE;

	std::unordered_map<EngineName, AllocatedBufferData> globalUniforms;
	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
};