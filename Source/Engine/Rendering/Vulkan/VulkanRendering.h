#pragma once

#include "EngineName.h"
#include "Frame.h"
#include "Rendering/RenderingInterface.h"
#include "VkContext.h"

#include <optional>
#include <SDL3/SDL_video.h>
#include <string>
#include <vector>
#include <vma/vk_mem_alloc.h>
#include <volk.h>
#include <unordered_map>

class RenderPipeline;

struct Entity;
struct MeshData;
struct MeshRenderData;
struct VulkanRenderData;

constexpr uint32_t NVIGIA_GPU = 0x10DE;
constexpr uint32_t AMD_GPU = 0x1002;
constexpr uint32_t INTEL_GPU = 0x8086;

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;
	std::optional<uint32_t> transferFamily;

	bool IsValid() const
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}

	bool SupportsTransfer() const
	{
		return transferFamily.has_value();
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct AllocatedBufferData
{
	VkBuffer buffer = VK_NULL_HANDLE;
	VmaAllocation memory = VK_NULL_HANDLE;
};

class VulkanRendering : public RenderingInterface
{
public:
	bool Initialize(int32_t inWidth = 0, int32_t inHeight = 0) override;
	void UnInitialize() override;

	void DrawFrame() override;
	void DrawSingle(const Entity& entity) override;
	void EndFrame() override;

	virtual void UpdateProjection(const glm::mat4& projection) override;
	virtual void UpdateView(const glm::mat4& view) override;

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage, VmaMemoryUsage usage, VmaAllocationCreateFlags properties, VkBuffer& buffer, VmaAllocation& bufferMemory) const;
	void CreateMeshVertexBuffer(const MeshData& meshData, MeshRenderData& outRenderData) override;
	void DestroyBuffer(GenericHandle buffer, GenericHandle bufferMemory) override;
	void DestroyMeshVertexBuffer(const MeshRenderData& renderData) override;
	void DestroyBuffer(VkBuffer buffer, VmaAllocation bufferMemory);

protected:
	void HandleWindowResized() override;
	void HandleWindowMinimized() override;

private:
	bool InitializeVulkan();
	bool CheckValidationSupport() const;

	bool CreateVulkanInstance();
	bool CreateSurface();
	bool PickPhysicalDevice();
	bool CreateLogicalDevice();
	bool CreateMemoryAllocator();
	bool CreateSwapChain();
	bool CreateImageViews();
	bool CreateRenderPass();
	bool CreateFrameBuffers();
	bool CreateCommandPool();
	bool CreateDescriptorPool();
	void CreateRenderFrames();

	void CreateSharedUniformBuffers();
	void CreateGlobalDescriptorSetLayouts();
	void CreateDescriptorSets();
	void CreateRenderPipelines();	
	void SetupDebugMessenger();

	void CleanupSwapChain();
	void CleanupSharedUniformBuffers();
	void CleanupPendingDestroyBuffers();
	void RecreateSwapChain();

	void RecordCommandBuffer(uint32_t imageIndex);
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcoffset, VkDeviceSize dstOffset);

	bool IsDeviceSuitable(VkPhysicalDevice device) const;
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;
	int32_t RateDeviceSuitability(VkPhysicalDevice device) const;
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) const;
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	std::vector<const char*> GetRequiredExtensions() const;

	VkContext context;

	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkImage> swapChainImages;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	uint32_t currentFrame = 0;

	std::unordered_map<EngineName, RenderPipeline*> renderPipelines;
	
	// Global descriptor layouts
	std::vector<VkDescriptorSet> globalDescriptorSets;

	/// <summary>
	/// These buffers are shared between multiple shaders
	/// An example would be the projection and view
	/// </summary>
	std::unordered_map<EngineName, std::vector<AllocatedBufferData>> sharedUniformBuffers;

	std::vector<Frame> renderFrames;

	std::vector<AllocatedBufferData> buffersPendingDelete;

	std::vector<const char*> deviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	// **** Editor *****
	const std::vector<const char*> validationLayers =
	{
	"VK_LAYER_KHRONOS_validation"
	};

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif
};