#include "Frame.h"
#include "VkContext.h"

#include <iostream>
#include <vector>

Frame::Frame(const VkContext& context)
	: supportsTransfer(context.transferQueue != VK_NULL_HANDLE)
{
	const uint32_t buffersCount = supportsTransfer ? 2 : 1;
	std::vector<VkCommandBuffer> buffers(buffersCount);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = context.commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = buffersCount;

	
	if (supportsTransfer)
	{
		buffers.push_back(transferCommandBuffer);
	}

	if (vkAllocateCommandBuffers(context.device, &allocInfo, buffers.data()) != VK_SUCCESS)
	{
		std::cerr << "Failed to allocate command buffers!" << std::endl;
	}

	drawCommandBuffer = buffers[0];
	if (supportsTransfer)
	{
		transferCommandBuffer = buffers[1];
	}

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &transferFinishedSemaphore) != VK_SUCCESS ||
		vkCreateFence(context.device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS)
	{
		std::cerr << "Failed to create semaphores!" << std::endl;
	}
}

void Frame::Destroy(const VkContext& context)
{
	vkDestroySemaphore(context.device, imageAvailableSemaphore, nullptr);
	vkDestroySemaphore(context.device, renderFinishedSemaphore, nullptr);
	vkDestroySemaphore(context.device, transferFinishedSemaphore, nullptr);
	vkDestroyFence(context.device, inFlightFence, nullptr);
}
