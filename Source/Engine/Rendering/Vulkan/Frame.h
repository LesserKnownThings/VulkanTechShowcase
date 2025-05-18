#pragma once

#include <volk.h>

struct VkContext;

struct Frame
{
public:
	Frame() = default;
	Frame(const VkContext& context);
	
	void Destroy(const VkContext& context);

	VkCommandBuffer drawCommandBuffer = VK_NULL_HANDLE;

	VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
	VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
	VkFence inFlightFence = VK_NULL_HANDLE;

	VkCommandBuffer transferCommandBuffer = VK_NULL_HANDLE;
	VkSemaphore transferFinishedSemaphore = VK_NULL_HANDLE;

	const bool supportsTransfer = false;
};