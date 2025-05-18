#include "VulkanRendering.h"
#include "Frame.h"
#include "Pipelines/LitPipeline.h"
#include "PushConstant.h"
#include "Rendering/Vulkan/Pipelines/PipelineNames.h"
#include "RenderPipeline.h"
#include "Rendering/SharedUniforms.h"
#include "RenderUtilities.h"
#include "Utilities/FileHelper.h"
#include "VulkanRenderData.h"

// ECS
#include "AssetManager/Model/MeshData.h"
#include "ECS/Systems/MeshSystem.h"
#include "ECS/Systems/TransformSystem.h"

#include <algorithm>
#include <cassert>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <map>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <set>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1004000
#include <vma/vk_mem_alloc.h>

#define VOLK_IMPLEMENTATION 
#include <volk.h>

constexpr int32_t MAX_FRAMES_IN_FLIGHT = 2;

namespace Utilities
{
	VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
		return VK_FALSE;
	}
}

bool VulkanRendering::Initialize(int32_t inWidth, int32_t inHeight)
{
	RenderingInterface::Initialize(inWidth, inHeight);

	window = SDL_CreateWindow(applicationName.c_str(), width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
	aspectRatio = static_cast<float>(width) / static_cast<float>(height);

	if (window == nullptr)
	{
		const char* message = SDL_GetError();
		std::cout << message << std::endl;
	}

	return window != nullptr && InitializeVulkan();
}

bool VulkanRendering::InitializeVulkan()
{
	bool success = CreateVulkanInstance();
	success &= CreateSurface();
	success &= PickPhysicalDevice();
	success &= CreateLogicalDevice();
	success &= CreateMemoryAllocator();
	success &= CreateSwapChain();
	success &= CreateImageViews();
	success &= CreateRenderPass();
	success &= CreateFrameBuffers();
	success &= CreateCommandPool();
	success &= CreateDescriptorPool();

	CreateSharedUniformBuffers();
	CreateGlobalDescriptorSetLayouts();
	CreateDescriptorSets();
	CreateRenderFrames();
	CreateRenderPipelines();
	SetupDebugMessenger();

	return success;
}

bool VulkanRendering::CheckValidationSupport() const
{
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

bool VulkanRendering::CreateVulkanInstance()
{
	if (volkInitialize() != VK_SUCCESS)
	{
		std::cerr << "Failed to initialize volk!" << std::endl;
		return false;
	}

	// TODO move this to a utility class for debugging
	if (enableValidationLayers && !CheckValidationSupport())
	{
		throw std::runtime_error("Validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = applicationName.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_4;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	uint32_t extensionCount = 0;
	const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);

	createInfo.enabledExtensionCount = extensionCount;
	createInfo.ppEnabledExtensionNames = extensions;

	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateInstance(&createInfo, nullptr, &context.instance) != VK_SUCCESS)
	{
		std::cerr << "Failed to create vulkan instance!" << std::endl;
		return false;
	}

	volkLoadInstance(context.instance);
	return true;
}

bool VulkanRendering::CreateSurface()
{
	if (!SDL_Vulkan_CreateSurface(window, context.instance, nullptr, &context.surface))
	{
		std::cerr << "Failed to create window surface!" << std::endl;
		return false;
	}
	return true;
}

bool VulkanRendering::PickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(context.instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		std::cerr << "Failed to find GPUs with Vulkan support!" << std::endl;
		return false;
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(context.instance, &deviceCount, devices.data());

	std::multimap<int32_t, VkPhysicalDevice> deviceRatings;

	for (VkPhysicalDevice device : devices)
	{
		if (IsDeviceSuitable(device))
		{
			int32_t score = RateDeviceSuitability(device);
			deviceRatings.emplace(std::make_pair(score, device));
		}
	}

	context.physicalDevice = deviceRatings.rbegin()->second;

	if (context.physicalDevice == VK_NULL_HANDLE)
	{
		std::cerr << "Failed to find a suitable GPU!" << std::endl;
		return false;
	}

	return true;
}

bool VulkanRendering::CreateLogicalDevice()
{
	QueueFamilyIndices indices = FindQueueFamilies(context.physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };
	if (indices.transferFamily.has_value())
	{
		uniqueQueueFamilies.emplace(indices.transferFamily.value());
	}

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(context.physicalDevice, &createInfo, nullptr, &context.device) != VK_SUCCESS)
	{
		std::cerr << "Failed to create logical device!" << std::endl;
		return false;
	}

	vkGetDeviceQueue(context.device, indices.graphicsFamily.value(), 0, &context.graphicsQueue);
	vkGetDeviceQueue(context.device, indices.presentFamily.value(), 0, &context.presentQueue);

	if (indices.SupportsTransfer())
	{
		vkGetDeviceQueue(context.device, indices.transferFamily.value(), 0, &context.transferQueue);
	}

	volkLoadDevice(context.device);

	return true;
}

bool VulkanRendering::CreateMemoryAllocator()
{
	VmaVulkanFunctions vmaFunctions = {};
	vmaFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vmaFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo allocatorCreateInfo{};
	allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
	allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_4;
	allocatorCreateInfo.physicalDevice = context.physicalDevice;
	allocatorCreateInfo.device = context.device;
	allocatorCreateInfo.instance = context.instance;
	allocatorCreateInfo.pVulkanFunctions = &vmaFunctions;

	if (vmaCreateAllocator(&allocatorCreateInfo, &context.allocator) != VK_SUCCESS)
	{
		std::cerr << "Failed to create memory allocator!";
		return false;
	}

	return true;
}

bool VulkanRendering::CreateSwapChain()
{
	SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(context.physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = context.surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = FindQueueFamilies(context.physicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(context.device, &createInfo, nullptr, &context.swapChain) != VK_SUCCESS)
	{
		std::cerr << "failed to create swap chain!" << std::endl;
		return false;
	}

	vkGetSwapchainImagesKHR(context.device, context.swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(context.device, context.swapChain, &imageCount, swapChainImages.data());

	context.swapChainImageFormat = surfaceFormat.format;
	context.swapChainExtent = extent;

	return true;
}

bool VulkanRendering::CreateImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = context.swapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(context.device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
		{
			std::cerr << "failed to create image views!" << std::endl;
			return false;
		}
	}

	return true;
}

bool VulkanRendering::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = context.swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(context.device, &renderPassInfo, nullptr, &context.renderPass) != VK_SUCCESS)
	{
		std::cerr << "Failed to create render pass!" << std::endl;
		return false;
	}

	return true;
}

bool VulkanRendering::CreateFrameBuffers()
{
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++)
	{
		VkImageView attachments[] =
		{
			swapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = context.renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = context.swapChainExtent.width;
		framebufferInfo.height = context.swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(context.device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
		{
			std::cerr << "Failed to create framebuffer!" << std::endl;
			return false;
		}
	}

	return true;
}

bool VulkanRendering::CreateCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(context.physicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(context.device, &poolInfo, nullptr, &context.commandPool) != VK_SUCCESS)
	{
		std::cerr << "Failed to create command pool!" << std::endl;
		return false;
	}

	return true;
}

bool VulkanRendering::CreateDescriptorPool()
{
	std::vector<VkDescriptorPoolSize> poolSizes =
	{
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 50},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 30}
	};

	VkDescriptorPoolCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	info.poolSizeCount = poolSizes.size();
	info.pPoolSizes = poolSizes.data();
	info.maxSets = 100;

	if (vkCreateDescriptorPool(context.device, &info, nullptr, &context.descriptorPool) != VK_SUCCESS)
	{
		std::cerr << "Failed to create descriptor pool!" << std::endl;
		return false;
	}

	return true;
}

void VulkanRendering::CreateRenderFrames()
{
	for (int32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		renderFrames.emplace_back(Frame{ context });
	}
}

void VulkanRendering::CreateGlobalDescriptorSetLayouts()
{
	std::vector<VkDescriptorSetLayoutBinding> bindings
	{
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.pImmutableSamplers = nullptr
		},
		{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr
		}
	};

	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	createInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(context.device, &createInfo, nullptr, &context.globalDescriptorSetLayout) != VK_SUCCESS)
	{
		std::cerr << "Failed to create descriptor set layout for the light!" << std::endl;
	}
}

void VulkanRendering::CreateDescriptorSets()
{
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, context.globalDescriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = context.descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	globalDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(context.device, &allocInfo, globalDescriptorSets.data()) != VK_SUCCESS)
	{
		std::cerr << "Failed to allocate global descriptor sets!" << std::endl;
	}

	for (int32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = sharedUniformBuffers[SharedUniforms::MatricesUniform][i].buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet descriptorWrite[2] = {};
		descriptorWrite[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite[0].dstSet = globalDescriptorSets[i];
		descriptorWrite[0].dstBinding = 0;
		descriptorWrite[0].dstArrayElement = 0;
		descriptorWrite[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite[0].descriptorCount = 1;
		descriptorWrite[0].pBufferInfo = &bufferInfo;
		descriptorWrite[0].pImageInfo = nullptr;
		descriptorWrite[0].pTexelBufferView = nullptr;

		descriptorWrite[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite[1].dstSet = globalDescriptorSets[i];
		descriptorWrite[1].dstBinding = 1;
		descriptorWrite[1].dstArrayElement = 0;
		descriptorWrite[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite[1].descriptorCount = 1;
		descriptorWrite[1].pBufferInfo = &bufferInfo;
		descriptorWrite[1].pImageInfo = nullptr;
		descriptorWrite[1].pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(context.device, 2, descriptorWrite, 0, nullptr);
	}
}

void VulkanRendering::CreateRenderPipelines()
{
	LitPipeline* litPipeline = new LitPipeline();
	litPipeline->Initialize(context);
	renderPipelines.emplace(PipelineNames::Lit, litPipeline);
}

void VulkanRendering::CreateSharedUniformBuffers()
{
	std::vector<AllocatedBufferData> matricesBuffers(MAX_FRAMES_IN_FLIGHT);

	for (AllocatedBufferData& bufferData : matricesBuffers)
	{
		CreateBuffer(
			sizeof(CameraMatrices),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
			bufferData.buffer,
			bufferData.memory
		);
	}

	sharedUniformBuffers.emplace(SharedUniforms::MatricesUniform, matricesBuffers);
}

void VulkanRendering::SetupDebugMessenger()
{
	if (!enableValidationLayers)
	{
		return;
	}

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = Utilities::DebugCallback;
	createInfo.pUserData = nullptr;
}

void VulkanRendering::CleanupSwapChain()
{
	vkDeviceWaitIdle(context.device);

	for (size_t i = 0; i < swapChainFramebuffers.size(); i++)
	{
		vkDestroyFramebuffer(context.device, swapChainFramebuffers[i], nullptr);
	}

	for (size_t i = 0; i < swapChainImageViews.size(); i++)
	{
		vkDestroyImageView(context.device, swapChainImageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(context.device, context.swapChain, nullptr);
}

void VulkanRendering::CleanupSharedUniformBuffers()
{
	for (const auto& it : sharedUniformBuffers)
	{
		const std::vector<AllocatedBufferData>& data = it.second;

		for (const AllocatedBufferData& bufferData : data)
		{
			vkDestroyBuffer(context.device, bufferData.buffer, nullptr);
			vmaFreeMemory(context.allocator, bufferData.memory);
		}
	}
}

void VulkanRendering::CleanupPendingDestroyBuffers()
{
	for (AllocatedBufferData& bufferData : buffersPendingDelete)
	{
		vmaDestroyBuffer(context.allocator, bufferData.buffer, bufferData.memory);
	}
	buffersPendingDelete.clear();
}

void VulkanRendering::RecreateSwapChain()
{
	CleanupSwapChain();

	CreateSwapChain();
	CreateImageViews();
	CreateFrameBuffers();
}

void VulkanRendering::HandleWindowResized()
{
	RenderingInterface::HandleWindowResized();

	RecreateSwapChain();
}

void VulkanRendering::HandleWindowMinimized()
{
	uint64_t flags = SDL_GetWindowFlags(window);

	while (flags & SDL_WINDOW_MINIMIZED)
	{
		//Engine::Get()->GetInputSystem()->RunEvents();
		flags = SDL_GetWindowFlags(window);
	}

	RecreateSwapChain();
}

bool VulkanRendering::IsDeviceSuitable(VkPhysicalDevice device) const
{
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	QueueFamilyIndices indices = FindQueueFamilies(device);

	bool extensionsSupported = CheckDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return indices.IsValid() && extensionsSupported && swapChainAdequate;
}

bool VulkanRendering::CheckDeviceExtensionSupport(VkPhysicalDevice device) const
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

int32_t VulkanRendering::RateDeviceSuitability(VkPhysicalDevice device) const
{
	int32_t rating = 0;

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	{
		rating += 1000;
	}

	rating += deviceProperties.limits.maxImageDimension2D;

	return rating;
}

QueueFamilyIndices VulkanRendering::FindQueueFamilies(VkPhysicalDevice device) const
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (!indices.graphicsFamily.has_value() && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i;
		}

		if (!indices.transferFamily.has_value() && (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))
		{
			indices.transferFamily = i;
		}

		if (indices.graphicsFamily.has_value() && indices.transferFamily.has_value())
		{
			break;
		}

		i++;
	}

	VkBool32 presentSupport = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(device, indices.graphicsFamily.value(), context.surface, &presentSupport);

	if (presentSupport)
	{
		indices.presentFamily = indices.graphicsFamily.value();
	}

	return indices;
}

SwapChainSupportDetails VulkanRendering::QuerySwapChainSupport(VkPhysicalDevice device) const
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, context.surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, context.surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, context.surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, context.surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, context.surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR VulkanRendering::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR VulkanRendering::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRendering::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}

	int32_t width, height;
	SDL_GetWindowSize(window, &width, &height);

	VkExtent2D actualExtent =
	{
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
	};

	actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return actualExtent;
}

std::vector<const char*> VulkanRendering::GetRequiredExtensions() const
{
	uint32_t extensionCount = 0;
	const char* const* extensionNames = SDL_Vulkan_GetInstanceExtensions(&extensionCount);

	std::vector<const char*> extensions(extensionNames, extensionNames + extensionCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

void VulkanRendering::UnInitialize()
{
	CleanupSwapChain();
	CleanupSharedUniformBuffers();
	CleanupPendingDestroyBuffers();

	vkDestroyDescriptorSetLayout(context.device, context.globalDescriptorSetLayout, nullptr);

	for (Frame& renderFrame : renderFrames)
	{
		renderFrame.Destroy(context);
	}
	renderFrames.clear();

	for (auto& pipeline : renderPipelines)
	{
		pipeline.second->DestroyPipeline();
		delete pipeline.second;
	}
	renderPipelines.clear();

	vkDestroyDescriptorPool(context.device, context.descriptorPool, nullptr);
	vkDestroyCommandPool(context.device, context.commandPool, nullptr);
	vkDestroyRenderPass(context.device, context.renderPass, nullptr);
	vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
	vmaDestroyAllocator(context.allocator);
	vkDestroyDevice(context.device, nullptr);
	vkDestroyInstance(context.instance, nullptr);

	RenderingInterface::UnInitialize();
}

void VulkanRendering::DrawFrame()
{
	Frame& frame = renderFrames[currentFrame];

	vkWaitForFences(context.device, 1, &frame.inFlightFence, VK_TRUE, UINT64_MAX);
	vkResetFences(context.device, 1, &frame.inFlightFence);

	uint32_t imageIndex;
	vkAcquireNextImageKHR(context.device, context.swapChain, UINT64_MAX, frame.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	RecordCommandBuffer(imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { frame.imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &frame.drawCommandBuffer;

	VkSemaphore signalSemaphores[] = { frame.renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, frame.inFlightFence) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { context.swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	vkQueuePresentKHR(context.presentQueue, &presentInfo);

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRendering::DrawSingle(const Entity& entity)
{
	Frame& frame = renderFrames[currentFrame];
	RenderPipeline* pipeline = renderPipelines[PipelineNames::Lit];

	vkCmdBindPipeline(frame.drawCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetPipeline());

	vkCmdBindDescriptorSets(frame.drawCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, pipeline->GetDescriptorSetCount(), &globalDescriptorSets[currentFrame], 0, nullptr);

	glm::mat4 model;
	if (TransformSystem::Get().TryGetModel(entity, model))
	{
		SingleEntityConstant sec{ model };
		vkCmdPushConstants(frame.drawCommandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(SingleEntityConstant), &sec);
	}

	MeshData meshData;
	MeshRenderData renderData;
	if (MeshSystem::Get().TryGetData(entity, &meshData, &renderData))
	{
		VkDeviceSize offset = 0;

		VkBuffer vertexBuffer = reinterpret_cast<VkBuffer>(std::get<uintptr_t>(renderData.vertexBuffer));
		VkBuffer indexBuffer = reinterpret_cast<VkBuffer>(std::get<uintptr_t>(renderData.indexBuffer));

		vkCmdBindVertexBuffers(frame.drawCommandBuffer, 0, 1, &vertexBuffer, &offset);
		vkCmdBindIndexBuffer(frame.drawCommandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(frame.drawCommandBuffer, static_cast<uint32_t>(meshData.indicesCount), 1, 0, 0, 0);
	}
}

void VulkanRendering::EndFrame()
{
	VkResult fenceStatus = vkGetFenceStatus(context.device, renderFrames[currentFrame].inFlightFence);
	if (fenceStatus == VK_SUCCESS)
	{
		CleanupPendingDestroyBuffers();
	}
}

void VulkanRendering::UpdateProjection(const glm::mat4& projection)
{
	// We rarely update projection so we need to update both in flight frames
	auto it = sharedUniformBuffers.find(SharedUniforms::MatricesUniform);
	if (it != sharedUniformBuffers.end())
	{
		for (int32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			AllocatedBufferData& bufferData = it->second[i];

			void* data;
			vmaMapMemory(context.allocator, bufferData.memory, &data);
			memcpy(data, glm::value_ptr(projection), sizeof(glm::mat4));
			vmaUnmapMemory(context.allocator, bufferData.memory);
		}
	}
}

void VulkanRendering::UpdateView(const glm::mat4& view)
{
	// The view updates pretty often os only 1 frame needs to be updated
	auto it = sharedUniformBuffers.find(SharedUniforms::MatricesUniform);
	if (it != sharedUniformBuffers.end())
	{
		AllocatedBufferData& bufferData = it->second[currentFrame];

		void* data;
		vmaMapMemory(context.allocator, bufferData.memory, &data);
		memcpy(static_cast<char*>(data) + sizeof(glm::mat4), glm::value_ptr(view), sizeof(glm::mat4));
		vmaUnmapMemory(context.allocator, bufferData.memory);
	}
}

void VulkanRendering::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage, VmaMemoryUsage usage, VmaAllocationCreateFlags properties, VkBuffer& buffer, VmaAllocation& bufferMemory) const
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = bufferUsage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocInfo = {};
	allocInfo.usage = usage;
	allocInfo.flags = properties;

	if (vmaCreateBuffer(context.allocator, &bufferInfo, &allocInfo, &buffer, &bufferMemory, nullptr) != VK_SUCCESS)
	{
		std::cerr << "Failed to create buffer!" << std::endl;
	}
}

void VulkanRendering::CreateMeshVertexBuffer(const MeshData& meshData, MeshRenderData& outRenderData)
{
	const VkDeviceSize verticesBufferSize = sizeof(Vertex) * meshData.verticesCount;
	const VkDeviceSize indicesBufferSize = sizeof(uint32_t) * meshData.indicesCount;
	const VkDeviceSize stagingBufferSize = verticesBufferSize + indicesBufferSize;

	VkBuffer stagingBuffer;
	VmaAllocation staginBufferMemory;

	CreateBuffer(
		stagingBufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_AUTO,
		VMA_MEMORY_USAGE_AUTO_PREFER_HOST | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		stagingBuffer,
		staginBufferMemory
	);

	void* data;
	vmaMapMemory(context.allocator, staginBufferMemory, &data);
	memcpy(data, meshData.vertices.data(), static_cast<size_t>(verticesBufferSize));
	memcpy(reinterpret_cast<char*>(data) + verticesBufferSize, meshData.indices.data(), static_cast<size_t>(indicesBufferSize));
	vmaUnmapMemory(context.allocator, staginBufferMemory);

	VkBuffer vertexBuffer;
	VkBuffer indexBuffer;

	VmaAllocation vertexMemory;
	VmaAllocation indexMemory;

	CreateBuffer(
		verticesBufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VMA_MEMORY_USAGE_AUTO,
		VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
		vertexBuffer,
		vertexMemory
	);

	CreateBuffer(
		indicesBufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VMA_MEMORY_USAGE_AUTO,
		VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
		indexBuffer,
		indexMemory
	);

	// Vertex buffer
	CopyBuffer(stagingBuffer, vertexBuffer, verticesBufferSize, 0, 0);

	// Indices buffer
	CopyBuffer(stagingBuffer, indexBuffer, indicesBufferSize, verticesBufferSize, 0);

	outRenderData.vertexBuffer = reinterpret_cast<uintptr_t>(vertexBuffer);
	outRenderData.indexBuffer = reinterpret_cast<uintptr_t>(indexBuffer);
	outRenderData.vertexMemory = reinterpret_cast<uintptr_t>(vertexMemory);
	outRenderData.indexMemory = reinterpret_cast<uintptr_t>(indexMemory);

	vmaDestroyBuffer(context.allocator, stagingBuffer, staginBufferMemory);
}

void VulkanRendering::DestroyBuffer(GenericHandle buffer, GenericHandle bufferMemory)
{
	VkBuffer internalBuffer = reinterpret_cast<VkBuffer>(std::get<uintptr_t>(buffer));
	VmaAllocation internalBufferMemory = reinterpret_cast<VmaAllocation>(std::get<uintptr_t>(bufferMemory));

	buffersPendingDelete.push_back(AllocatedBufferData{ internalBuffer, internalBufferMemory });
}

void VulkanRendering::DestroyMeshVertexBuffer(const MeshRenderData& renderData)
{
	DestroyBuffer(renderData.vertexBuffer, renderData.vertexMemory);
	DestroyBuffer(renderData.indexBuffer, renderData.indexMemory);
}

void VulkanRendering::DestroyBuffer(VkBuffer buffer, VmaAllocation bufferMemory)
{
	buffersPendingDelete.push_back(AllocatedBufferData{ buffer, bufferMemory });
}

void VulkanRendering::RecordCommandBuffer(uint32_t imageIndex)
{
	Frame& frame = renderFrames[currentFrame];

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0; // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(frame.drawCommandBuffer, &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = context.renderPass;
	renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = context.swapChainExtent;

	VkClearValue clearColor = { {{0.1f, 0.1f, 0.1f, 1.0f}} };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(frame.drawCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(context.swapChainExtent.width);
	viewport.height = static_cast<float>(context.swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(frame.drawCommandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = context.swapChainExtent;
	vkCmdSetScissor(frame.drawCommandBuffer, 0, 1, &scissor);

	RenderTasks();

	vkCmdEndRenderPass(frame.drawCommandBuffer);

	if (vkEndCommandBuffer(frame.drawCommandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}

void VulkanRendering::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcoffset, VkDeviceSize dstOffset)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = context.commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer drawCommandBuffer;
	vkAllocateCommandBuffers(context.device, &allocInfo, &drawCommandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(drawCommandBuffer, &beginInfo);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = srcoffset;
	copyRegion.dstOffset = dstOffset;
	copyRegion.size = size;
	vkCmdCopyBuffer(drawCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(drawCommandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &drawCommandBuffer;

	vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(context.graphicsQueue);

	vkFreeCommandBuffers(context.device, context.commandPool, 1, &drawCommandBuffer);
}
