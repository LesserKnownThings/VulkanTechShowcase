#include "VulkanRendering.h"
#include "AssetManager/AssetManager.h"
#include "AssetManager/Model/Model.h"
#include "AssetManager/Model/MeshData.h"
#include "AssetManager/Texture/TextureData.h"
#include "ECS/Components/Components.h"
#include "ECS/Systems/MaterialSystem.h"
#include "Engine.h"
#include "Frame.h"
#include "Input/InputSystem.h"
#include "Pipelines/PBRPipeline.h"
#include "PushConstant.h"
#include "Rendering/Descriptors/DescriptorInfo.h"
#include "Rendering/Descriptors/DescriptorRegistry.h"
#include "Rendering/Light/Light.h"
#include "Rendering/SharedUniforms.h"
#include "RenderPipeline.h"
#include "RenderUtilities.h"
#include "Utilities/FileHelper.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <map>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <set>
#include <thread>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1004000
#include <vma/vk_mem_alloc.h>

#define VOLK_IMPLEMENTATION 
#include <volk.h>

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
	CreateColorResources();
	CreateDepthResources();
	success &= CreateFrameBuffers();
	success &= CreateCommandPools();
	success &= CreateDescriptorPool();

	CreateRenderFrames();
	CreateDescriptorRegistry();
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
	context.msaaSamples = GetMaxUsableSampleCount();

	if (context.physicalDevice == VK_NULL_HANDLE)
	{
		std::cerr << "Failed to find a suitable GPU!" << std::endl;
		return false;
	}

	return true;
}

bool VulkanRendering::CreateLogicalDevice()
{
	context.familyIndices = FindQueueFamilies(context.physicalDevice);
	QueueFamilyIndices& indices = context.familyIndices;

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
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE;

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

	swapChainImageFormat = surfaceFormat.format;
	context.swapChainExtent = extent;

	return true;
}

bool VulkanRendering::CreateImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		swapChainImageViews[i] = CreateImageView(
			swapChainImages[i],
			swapChainImageFormat,
			VK_IMAGE_ASPECT_COLOR_BIT,
			1
		);
	}

	return true;
}

bool VulkanRendering::CreateRenderPass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = context.msaaSamples;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = FindDepthFormat();
	depthAttachment.samples = context.msaaSamples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachmentResolve{};
	colorAttachmentResolve.format = swapChainImageFormat;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentResolveRef{};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.pResolveAttachments = &colorAttachmentResolveRef;

	std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(context.device, &renderPassInfo, nullptr, &context.renderPass) != VK_SUCCESS)
	{
		std::cerr << "Failed to create render pass!" << std::endl;
		return false;
	}

	return true;
}

void VulkanRendering::CreateColorResources()
{
	VkFormat colorFormat = swapChainImageFormat;

	VkImage image;
	VkImageView view;
	VmaAllocation memory;

	CreateImage(
		context.swapChainExtent.width,
		context.swapChainExtent.height,
		1,
		context.msaaSamples,
		colorFormat,
		VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		image,
		memory);

	view = CreateImageView(image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	colorImage.image = RenderUtilities::ImageToGenericHandle(image);
	colorImage.view = RenderUtilities::ImageViewToGenericHandle(view);
	colorImage.memory = RenderUtilities::AllocationToGenericHandle(memory);
}

bool VulkanRendering::CreateFrameBuffers()
{
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++)
	{
		std::array<VkImageView, 3> attachments =
		{
			RenderUtilities::GenericHandleToImageView(colorImage.view),
			RenderUtilities::GenericHandleToImageView(depthImage.view),
			swapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = context.renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
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

bool VulkanRendering::CreateCommandPools()
{
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(context.physicalDevice);

	VkCommandPoolCreateInfo graphicsPoolInfo{};
	graphicsPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	graphicsPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	graphicsPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(context.device, &graphicsPoolInfo, nullptr, &context.graphicsCommandPool) != VK_SUCCESS)
	{
		std::cerr << "Failed to create graphics command pool!" << std::endl;
		return false;
	}

	VkCommandPoolCreateInfo transferPoolinfo{};
	transferPoolinfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	transferPoolinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	transferPoolinfo.queueFamilyIndex = queueFamilyIndices.transferFamily.value();

	if (vkCreateCommandPool(context.device, &transferPoolinfo, nullptr, &context.transferCommandPool) != VK_SUCCESS)
	{
		std::cerr << "Failed to create transfer command pool!" << std::endl;
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

void VulkanRendering::CreateGlobalUniformBuffers(std::array<AllocatedBuffer, MAX_FRAMES_IN_FLIGHT>& buffers)
{
	for (int32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		VkBuffer buffer;
		VmaAllocation memory;

		CreateBuffer(
			sizeof(CameraMatrices),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
			buffer,
			memory
		);

		buffers[i].buffer = RenderUtilities::BufferToGenericHandle(buffer);
		buffers[i].memory = RenderUtilities::AllocationToGenericHandle(memory);
	}
}

void VulkanRendering::CreateLightBuffer(AllocatedBuffer& outBuffer)
{
	VkBuffer buffer;
	VmaAllocation memory;

	CreateBuffer(
		sizeof(LightBufferLayout) * MAX_LIGHTS,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		buffer,
		memory
	);

	outBuffer.buffer = RenderUtilities::BufferToGenericHandle(buffer);
	outBuffer.memory = RenderUtilities::AllocationToGenericHandle(memory);
}

void VulkanRendering::CreateGlobalDescriptorLayouts(DescriptorSetLayoutInfo& globalLayout, DescriptorSetLayoutInfo& lightLayout)
{
	VkDescriptorSetLayoutBinding cameraMatricesBinding{};
	cameraMatricesBinding.binding = 0;
	cameraMatricesBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	cameraMatricesBinding.descriptorCount = 1;
	cameraMatricesBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo cameraCreateInfo{};
	cameraCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	cameraCreateInfo.bindingCount = 1;
	cameraCreateInfo.pBindings = &cameraMatricesBinding;

	VkDescriptorSetLayout cameraMatricesLayout;
	if (vkCreateDescriptorSetLayout(context.device, &cameraCreateInfo, nullptr, &cameraMatricesLayout) != VK_SUCCESS)
	{
		std::cerr << "Failed to create descriptor set layout for the camera matrices!" << std::endl;
	}
	globalLayout.layout = RenderUtilities::DescriptorSetLayoutToGenericHandle(cameraMatricesLayout);


	VkDescriptorSetLayoutBinding lightBinding{};
	lightBinding.binding = 0;
	lightBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	lightBinding.descriptorCount = 1;
	lightBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo lightCreateInfo{};
	lightCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	lightCreateInfo.bindingCount = 1;
	lightCreateInfo.pBindings = &lightBinding;

	VkDescriptorSetLayout lightDescriptorLayout;
	if (vkCreateDescriptorSetLayout(context.device, &lightCreateInfo, nullptr, &lightDescriptorLayout) != VK_SUCCESS)
	{
		std::cerr << "Failed to create descriptor set layout for the light!" << std::endl;
	}
	lightLayout.layout = RenderUtilities::DescriptorSetLayoutToGenericHandle(lightDescriptorLayout);
}

void VulkanRendering::AllocateGlobalDescriptorSet(const DescriptorSetLayoutInfo& layoutInfo, std::array<GenericHandle, MAX_FRAMES_IN_FLIGHT>& outDescriptorSets)
{
	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, RenderUtilities::GenericHandleToDescriptorSetLayout(layoutInfo.layout));

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = context.descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> globalDescriptorSets = {};

	if (vkAllocateDescriptorSets(context.device, &allocInfo, globalDescriptorSets.data()) != VK_SUCCESS)
	{
		std::cerr << "Failed to allocate global descriptor sets!" << std::endl;
	}

	for (int32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		outDescriptorSets[i] = RenderUtilities::DescriptorSetToGenericHandle(globalDescriptorSets[i]);
	}

	for (int32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = RenderUtilities::GenericHandleToBuffer(descriptorRegistry->GetMatricesBuffers()[i].buffer);
		bufferInfo.offset = 0;
		bufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet write = {};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = globalDescriptorSets[i];
		write.dstBinding = 0;
		write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write.descriptorCount = 1;
		write.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(context.device, 1, &write, 0, nullptr);
	}
}

void VulkanRendering::AllocateLightDescriptorSet(const DescriptorSetLayoutInfo& layoutInfo, GenericHandle& outDescriptorSet)
{
	VkDescriptorSetLayout layout = RenderUtilities::GenericHandleToDescriptorSetLayout(layoutInfo.layout);

	VkDescriptorSetAllocateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	info.descriptorPool = context.descriptorPool;
	info.descriptorSetCount = 1;
	info.pSetLayouts = &layout;

	VkDescriptorSet set;
	if (vkAllocateDescriptorSets(context.device, &info, &set) != VK_SUCCESS)
	{
		std::cerr << "Failed to allocate light descriptor set!" << std::endl;
	}
	outDescriptorSet = RenderUtilities::DescriptorSetToGenericHandle(set);

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = RenderUtilities::GenericHandleToBuffer(descriptorRegistry->GetLightBuffer().buffer);
	bufferInfo.offset = 0;
	bufferInfo.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet write = {};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = set;
	write.dstBinding = 0;
	write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	write.descriptorCount = 1;
	write.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(context.device, 1, &write, 0, nullptr);
}

void VulkanRendering::CreateRenderPipelines()
{
	PBRPipeline* litPipeline = new PBRPipeline();
	litPipeline->Initialize(context, this);
	renderPipelines.emplace(EPipelineType::PBR, litPipeline);
}

void VulkanRendering::CreateDepthResources()
{
	VkFormat depthFormat = FindDepthFormat();

	VkImage image;
	VkImageView view;
	VmaAllocation memory;

	CreateImage(
		context.swapChainExtent.width,
		context.swapChainExtent.height,
		1,
		context.msaaSamples,
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
		image,
		memory
	);

	view = CreateImageView(image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	depthImage.image = RenderUtilities::ImageToGenericHandle(image);
	depthImage.view = RenderUtilities::ImageViewToGenericHandle(view);
	depthImage.memory = RenderUtilities::AllocationToGenericHandle(memory);
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

	vmaDestroyImage(context.allocator,
		RenderUtilities::GenericHandleToImage(colorImage.image),
		RenderUtilities::GenericHandleToAllocation(colorImage.memory));

	vkDestroyImageView(context.device, RenderUtilities::GenericHandleToImageView(colorImage.view), nullptr);

	vmaDestroyImage(context.allocator,
		RenderUtilities::GenericHandleToImage(depthImage.image),
		RenderUtilities::GenericHandleToAllocation(depthImage.memory));

	vkDestroyImageView(context.device, RenderUtilities::GenericHandleToImageView(depthImage.view), nullptr);

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

void VulkanRendering::CleanupPendingDestroyBuffers()
{
	for (AllocatedBuffer& bufferData : buffersPendingDelete)
	{
		vmaDestroyBuffer(context.allocator,
			RenderUtilities::GenericHandleToBuffer(bufferData.buffer),
			RenderUtilities::GenericHandleToAllocation(bufferData.memory));
	}
	buffersPendingDelete.clear();

	for (AllocatedImage& images : imagesPendingDelete)
	{
		VkImage image = RenderUtilities::GenericHandleToImage(images.image);
		VmaAllocation memory = RenderUtilities::GenericHandleToAllocation(images.memory);
		VkImageView imageView = RenderUtilities::GenericHandleToImageView(images.view);
		VkSampler sampler = RenderUtilities::GenericHandleToImageSampler(images.sampler);

		vmaDestroyImage(context.allocator, image, memory);
		vkDestroyImageView(context.device, imageView, nullptr);

		// In some contexts the sampler might be null
		if (sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(context.device, sampler, nullptr);
		}
	}
}

void VulkanRendering::RecreateSwapChain()
{
	CleanupSwapChain();
	CreateSwapChain();
	CreateImageViews();
	CreateColorResources();
	CreateDepthResources();
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
		GameEngine->GetInputSystem()->RunEvents();
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

	return indices.IsValid() && extensionsSupported && swapChainAdequate && deviceFeatures.samplerAnisotropy;
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

VkFormat VulkanRendering::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(context.physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("Failed to find supported format!");
}

VkFormat VulkanRendering::FindDepthFormat() const
{
	return FindSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkSampleCountFlagBits VulkanRendering::GetMaxUsableSampleCount() const
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(context.physicalDevice, &physicalDeviceProperties);

	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;

	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}

void VulkanRendering::CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaAllocationCreateFlags memoryFlags, VkImage& outImage, VmaAllocation& outMemory)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = numSamples;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo memoryInfo{};
	memoryInfo.usage = VMA_MEMORY_USAGE_AUTO;
	memoryInfo.flags = memoryFlags;

	if (vmaCreateImage(context.allocator, &imageInfo, &memoryInfo, &outImage, &outMemory, nullptr) != VK_SUCCESS)
	{
		std::cerr << "Failed to create image!" << std::endl;
	}
}

VkImageView VulkanRendering::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
	VkImageViewCreateInfo imageViewInfo{};
	imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewInfo.image = image;
	imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewInfo.format = format;
	imageViewInfo.subresourceRange.aspectMask = aspectFlags;
	imageViewInfo.subresourceRange.baseMipLevel = 0;
	imageViewInfo.subresourceRange.levelCount = mipLevels;
	imageViewInfo.subresourceRange.baseArrayLayer = 0;
	imageViewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(context.device, &imageViewInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		std::cerr << "Failed to create image view!" << std::endl;
	}

	return imageView;
}

SingleTimeCommandBuffer VulkanRendering::BeginSingleTimeCommands(VkCommandPool commandPool, VkQueue queue)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(context.device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkFence fence;
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	if (vkCreateFence(context.device, &fenceInfo, nullptr, &fence) != VK_SUCCESS)
	{
		std::cerr << "Failed to create fence for single time command buffer!" << std::endl;
	}

	SingleTimeCommandBuffer sCommandBuffer{};
	sCommandBuffer.commandBuffer = commandBuffer;
	sCommandBuffer.commandPool = commandPool;
	sCommandBuffer.queue = queue;
	sCommandBuffer.fence = fence;
	return sCommandBuffer;
}

void VulkanRendering::EndSingleTimeCommands(const SingleTimeCommandBuffer& commandBuffer)
{
	vkEndCommandBuffer(commandBuffer.commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer.commandBuffer;

	vkQueueSubmit(commandBuffer.queue, 1, &submitInfo, commandBuffer.fence);
	vkWaitForFences(context.device, 1, &commandBuffer.fence, VK_TRUE, UINT64_MAX);
	vkDestroyFence(context.device, commandBuffer.fence, nullptr);
	vkFreeCommandBuffers(context.device, commandBuffer.commandPool, 1, &commandBuffer.commandBuffer);
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
	CleanupPendingDestroyBuffers();

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
	vkDestroyCommandPool(context.device, context.graphicsCommandPool, nullptr);
	vkDestroyCommandPool(context.device, context.transferCommandPool, nullptr);
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
	submitInfo.pCommandBuffers = &frame.commandBuffer;

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

void VulkanRendering::DrawSingle(const std::vector<entt::entity>& entities, const entt::registry& registry)
{
	Frame& frame = renderFrames[currentFrame];
	VkCommandBuffer cmdBuffer = frame.commandBuffer;

	for (const entt::entity& entity : entities)
	{
		const Transform& transform = registry.get<const Transform>(entity);
		const Material& material = registry.get<const Material>(entity);
		MaterialInstance materialInstance;
		GameEngine->GetMaterialSystem()->TryGetMaterialInstance(material.materialInstanceHandle, materialInstance);

		RenderPipeline* pipeline = renderPipelines[material.pipeline];

		pipeline->Bind(cmdBuffer);
		
		std::vector<VkDescriptorSet> sets =
		{
			RenderUtilities::GenericHandleToDescriptorSet(descriptorRegistry->GetGlobalDescriptorSets()[currentFrame]),
			RenderUtilities::GenericHandleToDescriptorSet(descriptorRegistry->GetLightDescriptorSet()),
			RenderUtilities::GenericHandleToDescriptorSet(materialInstance.descriptorSet)			
		};

		vkCmdBindDescriptorSets(cmdBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline->GetLayout(),
			0,
			sets.size(),
			sets.data(),
			0, nullptr);

		glm::mat4 transformModel = transform.ComputeModel();

		vkCmdPushConstants(cmdBuffer,
			pipeline->GetLayout(),
			VK_SHADER_STAGE_VERTEX_BIT,
			0,
			sizeof(glm::mat4),
			&transformModel);

		Mesh meshComp = registry.get<Mesh>(entity);
		const Model* model = AssetManager::Get().LoadAsset<Model>(meshComp.handle);
		MeshData meshData = model->GetMeshes()[0];
		MeshRenderData renderData = model->GetRenderData()[0];

		VkDeviceSize offsets[] = { 0 };
		VkBuffer vertexBuffer = RenderUtilities::GenericHandleToBuffer(renderData.vertex.buffer);
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, offsets);

		VkBuffer indexBuffer = RenderUtilities::GenericHandleToBuffer(renderData.index.buffer);
		vkCmdBindIndexBuffer(cmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(cmdBuffer,
			meshData.indicesCount,
			1,
			0,
			0,
			0);
	}
}

void VulkanRendering::DrawLight(const std::vector<entt::entity>& entities, const entt::registry& registry)
{

}

void VulkanRendering::EndFrame()
{
	VkResult fenceStatus = vkGetFenceStatus(context.device, renderFrames[currentFrame].inFlightFence);
	if (fenceStatus == VK_SUCCESS)
	{
		CleanupPendingDestroyBuffers();
	}
}

void VulkanRendering::AllocateMaterialDescriptorSet(EPipelineType pipeline, GenericHandle& outDescriptorSet)
{
	auto it = renderPipelines.find(pipeline);
	if (it != renderPipelines.end())
	{
		it->second->AllocateMaterialDescriptorSet(outDescriptorSet);
	}
}

void VulkanRendering::UpdateMaterialDescriptorSet(EPipelineType pipeline, const std::unordered_map<std::string, DescriptorDataProvider>& dataProviders)
{
	auto it = renderPipelines.find(pipeline);
	if (it != renderPipelines.end())
	{
		it->second->UpdateMaterialDescriptorSet(dataProviders);
	}
}

void VulkanRendering::UpdateProjection(const glm::mat4& projection)
{
	// We rarely update projection so we need to update both in flight frames
	for (int32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		VmaAllocation memory = RenderUtilities::GenericHandleToAllocation(descriptorRegistry->GetMatricesBuffers()[i].memory);

		void* data;
		vmaMapMemory(context.allocator, memory, &data);
		memcpy(data, glm::value_ptr(projection), sizeof(glm::mat4));
		vmaUnmapMemory(context.allocator, memory);
	}
}

void VulkanRendering::UpdateView(const glm::mat4& view)
{
	VmaAllocation memory = RenderUtilities::GenericHandleToAllocation(descriptorRegistry->GetMatricesBuffers()[currentFrame].memory);

	void* data;
	vmaMapMemory(context.allocator, memory, &data);
	memcpy(static_cast<char*>(data) + sizeof(glm::mat4), glm::value_ptr(view), sizeof(glm::mat4));
	vmaUnmapMemory(context.allocator, memory);

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
	/*auto func = [&]()
		{*/
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

	outRenderData.vertex.buffer = RenderUtilities::BufferToGenericHandle(vertexBuffer);
	outRenderData.index.buffer = RenderUtilities::BufferToGenericHandle(indexBuffer);
	outRenderData.vertex.memory = RenderUtilities::AllocationToGenericHandle(vertexMemory);
	outRenderData.index.memory = RenderUtilities::AllocationToGenericHandle(indexMemory);
	outRenderData.state = ERenderDataLoadState::Ready;

	vmaDestroyBuffer(context.allocator, stagingBuffer, staginBufferMemory);
	/*};

std::thread t{ func };
t.detach();*/
}

void VulkanRendering::CreateTextureBuffer(const TextureData& textureData, void* pixels, TextureRenderData& renderData)
{
	//auto func = [&](void* pixels)
		//{
	VkBuffer stagingBuffer;
	VmaAllocation stagingBufferMemory;

	const VkDeviceSize textureSize = static_cast<VkDeviceSize>(textureData.width) * static_cast<VkDeviceSize>(textureData.height) * 4;

	CreateBuffer(
		textureSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_MEMORY_USAGE_AUTO,
		VMA_MEMORY_USAGE_AUTO_PREFER_HOST | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		stagingBuffer,
		stagingBufferMemory
	);

	void* data;
	vmaMapMemory(context.allocator, stagingBufferMemory, &data);
	memcpy(data, pixels, static_cast<size_t>(textureSize));
	vmaUnmapMemory(context.allocator, stagingBufferMemory);
	free(pixels);

	VkImage imageBuffer;
	VmaAllocation imageMemory;
	CreateImage(
		textureData.width,
		textureData.height,
		textureData.mipLevels,
		VK_SAMPLE_COUNT_1_BIT,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
		imageBuffer,
		imageMemory);

	SingleTimeCommandBuffer commandBuffer = BeginSingleTimeCommands(context.transferCommandPool, context.transferQueue);

	TransitionImageLayout(
		commandBuffer.commandBuffer,
		imageBuffer,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		textureData.mipLevels);

	CopyBufferToImage(commandBuffer.commandBuffer, stagingBuffer, imageBuffer, textureData.width, textureData.height);
	EndSingleTimeCommands(commandBuffer);

	TransitionImageOwnership(
		imageBuffer,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		context.familyIndices.transferFamily.value(),
		context.familyIndices.graphicsFamily.value()
	);

	// Mipmaps already transfer the layout so no need to do another cmd
	GenerateMipmaps(imageBuffer, textureData.width, textureData.height, textureData.mipLevels);

	renderData.image.image = RenderUtilities::ImageToGenericHandle(imageBuffer);
	renderData.image.memory = RenderUtilities::AllocationToGenericHandle(imageMemory);

	vmaDestroyBuffer(context.allocator, stagingBuffer, stagingBufferMemory);

	VkImageViewCreateInfo imageViewInfo{};
	imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewInfo.image = imageBuffer;
	imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewInfo.subresourceRange.baseMipLevel = 0;
	imageViewInfo.subresourceRange.levelCount = 1;
	imageViewInfo.subresourceRange.baseArrayLayer = 0;
	imageViewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(context.device, &imageViewInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		std::cerr << "Failed to create image view!" << std::endl;
	}

	renderData.image.view = RenderUtilities::ImageViewToGenericHandle(imageView);

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;

	VkPhysicalDeviceProperties deviceProperties{};
	vkGetPhysicalDeviceProperties(context.physicalDevice, &deviceProperties);

	samplerInfo.maxAnisotropy = deviceProperties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	VkSampler sampler;
	if (vkCreateSampler(context.device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
	{
		std::cerr << "Failed tocreate image sampler!" << std::endl;
	}

	renderData.image.sampler = RenderUtilities::ImageSamplerToGenericHandle(sampler);
	renderData.state = ERenderDataLoadState::Ready;
	//};

/*std::thread t{ func, pixels };
t.detach();*/
}

void VulkanRendering::DestroyBuffer(GenericHandle buffer, GenericHandle bufferMemory)
{
	buffersPendingDelete.push_back(AllocatedBuffer{ buffer, bufferMemory });
}

void VulkanRendering::DestroyMeshVertexBuffer(const MeshRenderData& renderData)
{
	DestroyBuffer(renderData.vertex.buffer, renderData.vertex.memory);
	DestroyBuffer(renderData.index.buffer, renderData.index.memory);
}

void VulkanRendering::DestroyTextureBuffer(const TextureRenderData& renderData)
{
	imagesPendingDelete.push_back(renderData.image);
}

void VulkanRendering::DestroyBuffer(VkBuffer buffer, VmaAllocation bufferMemory)
{
	buffersPendingDelete.push_back(AllocatedBuffer{
		RenderUtilities::BufferToGenericHandle(buffer),
		RenderUtilities::AllocationToGenericHandle(bufferMemory)
		});
}

void VulkanRendering::RecordCommandBuffer(uint32_t imageIndex)
{
	Frame& frame = renderFrames[currentFrame];

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(frame.commandBuffer, &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = context.renderPass;
	renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = context.swapChainExtent;

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {0.1f, 0.1f, 0.1f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(frame.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(context.swapChainExtent.width);
	viewport.height = static_cast<float>(context.swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(frame.commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = context.swapChainExtent;
	vkCmdSetScissor(frame.commandBuffer, 0, 1, &scissor);

	RenderTasks();

	vkCmdEndRenderPass(frame.commandBuffer);

	if (vkEndCommandBuffer(frame.commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}

void VulkanRendering::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcoffset, VkDeviceSize dstOffset)
{
	SingleTimeCommandBuffer commandBuffer = BeginSingleTimeCommands(context.transferCommandPool, context.transferQueue);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = srcoffset;
	copyRegion.dstOffset = dstOffset;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer.commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndSingleTimeCommands(commandBuffer);
}

void VulkanRendering::CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent =
	{
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);
}

void VulkanRendering::TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels)
{
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	{
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);
}

void VulkanRendering::TransitionImageOwnership(VkImage imageBuffer, VkImageLayout layout, uint32_t srcFamilyIndex, uint32_t dstFamilyIndex)
{
	SingleTimeCommandBuffer commandBuffer = BeginSingleTimeCommands(context.transferCommandPool, context.transferQueue);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = layout;
	barrier.newLayout = layout;
	barrier.srcQueueFamilyIndex = srcFamilyIndex;
	barrier.dstQueueFamilyIndex = dstFamilyIndex;
	barrier.image = imageBuffer;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = 0; // nothing to wait for on destination yet

	vkCmdPipelineBarrier(
		commandBuffer.commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	EndSingleTimeCommands(commandBuffer);

	SingleTimeCommandBuffer aquireCommandBuffer = BeginSingleTimeCommands(context.graphicsCommandPool, context.graphicsQueue);

	VkImageMemoryBarrier aquireBarrier{};
	aquireBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	aquireBarrier.oldLayout = layout;
	aquireBarrier.newLayout = layout;
	aquireBarrier.srcQueueFamilyIndex = srcFamilyIndex;
	aquireBarrier.dstQueueFamilyIndex = dstFamilyIndex;
	aquireBarrier.image = imageBuffer;
	aquireBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	aquireBarrier.subresourceRange.baseMipLevel = 0;
	aquireBarrier.subresourceRange.levelCount = 1;
	aquireBarrier.subresourceRange.baseArrayLayer = 0;
	aquireBarrier.subresourceRange.layerCount = 1;
	aquireBarrier.srcAccessMask = 0;
	aquireBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(
		aquireCommandBuffer.commandBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0, nullptr,
		0, nullptr,
		1, &aquireBarrier
	);

	EndSingleTimeCommands(aquireCommandBuffer);
}

void VulkanRendering::GenerateMipmaps(VkImage image, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
	SingleTimeCommandBuffer commandBuffer = BeginSingleTimeCommands(context.graphicsCommandPool, context.graphicsQueue);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	for (uint32_t i = 1; i < mipLevels; i++)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer.commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer.commandBuffer,
			image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &blit,
			VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer.commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer.commandBuffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		0, nullptr,
		0, nullptr,
		1, &barrier);

	EndSingleTimeCommands(commandBuffer);
}
