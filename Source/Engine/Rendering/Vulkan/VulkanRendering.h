#pragma once

#include "EngineName.h"
#include "Frame.h"
#include "Rendering/AbstractData.h"
#include "Rendering/RenderingInterface.h"
#include "VkContext.h"

#include <SDL3/SDL_video.h>
#include <string>
#include <vector>
#include <vma/vk_mem_alloc.h>
#include <volk.h>
#include <unordered_map>

class RenderPipeline;

struct MeshData;
struct MeshRenderData;

constexpr uint32_t NVIGIA_GPU = 0x10DE;
constexpr uint32_t AMD_GPU = 0x1002;
constexpr uint32_t INTEL_GPU = 0x8086;

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct SingleTimeCommandBuffer
{
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	VkFence fence = VK_NULL_HANDLE;
	VkCommandPool commandPool = VK_NULL_HANDLE;
	VkQueue queue = VK_NULL_HANDLE;
};

class VulkanRendering : public RenderingInterface
{
public:
	bool Initialize(int32_t inWidth = 0, int32_t inHeight = 0) override;
	void UnInitialize() override;

	void DrawFrame() override;
	void DrawSingle(const std::vector<entt::entity>& entities, const entt::registry& registry) override;
	void DrawLight(const std::vector<entt::entity>& entities, const entt::registry& registry) override;
	void EndFrame() override;

	// Material descriptors
	void AllocateMaterialDescriptorSet(EPipelineType pipeline, GenericHandle& outDescriptorSet) override;
	void UpdateMaterialDescriptorSet(EPipelineType pipeline, const std::unordered_map<std::string, DescriptorDataProvider>& dataProviders) override;
	// *******************

	/// Global descriptors
	void CreateGlobalUniformBuffers(std::array<AllocatedBuffer, MAX_FRAMES_IN_FLIGHT>& buffers) override;
	void CreateLightBuffer(AllocatedBuffer& outBuffer) override;
	void CreateGlobalDescriptorLayouts(DescriptorSetLayoutInfo& globalLayout, DescriptorSetLayoutInfo& lightLayout) override;
	void AllocateGlobalDescriptorSet(const DescriptorSetLayoutInfo& layoutInfo, std::array<GenericHandle, MAX_FRAMES_IN_FLIGHT>& outDescriptorSets) override;
	void AllocateLightDescriptorSet(const DescriptorSetLayoutInfo& layoutInfo, GenericHandle& outDescriptorSet) override;
	void DestroyDescriptorSetLayout(const DescriptorSetLayoutInfo& layoutInfo) override;
	// *******************

	virtual void UpdateProjection(const glm::mat4& projection) override;
	virtual void UpdateView(const glm::mat4& view) override;

	// Buffer manips
	void CreateMeshVertexBuffer(const MeshData& meshData, MeshRenderData& outRenderData) override;
	void CreateTextureBuffer(const TextureData& textureData, void* pixels, TextureRenderData& renderData) override;

	void UpdateBuffer(AllocatedBuffer buffer, uint32_t offset, uint32_t range, void* dataToCopy) override;

	void DestroyBuffer(AllocatedBuffer buffer) override;
	void DestroyTexture(AllocatedTexture texture) override;
	// ************

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
	void CreateColorResources();
	void CreateDepthResources();
	bool CreateFrameBuffers();
	bool CreateCommandPools();
	bool CreateDescriptorPool();
	void CreateRenderFrames();
	void CreateRenderPipelines();
	void SetupDebugMessenger();

	void CleanupSwapChain();
	void CleanupPendingDestroyBuffers();
	void RecreateSwapChain();

	void RecordCommandBuffer(uint32_t imageIndex);

	bool IsDeviceSuitable(VkPhysicalDevice device) const;
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;

	int32_t RateDeviceSuitability(VkPhysicalDevice device) const;
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) const;
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	// Helper functions
	void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaAllocationCreateFlags memoryFlags, VkImage& outImage, VmaAllocation& outMemory);

	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage, VmaMemoryUsage usage, VmaAllocationCreateFlags properties, VkBuffer& buffer, VmaAllocation& bufferMemory) const;
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcoffset, VkDeviceSize dstOffset);

	void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void TransitionImageOwnership(VkImage imageBuffer, VkImageLayout layout, uint32_t srcFamilyIndex, uint32_t dstFamilyIndex);

	void GenerateMipmaps(VkImage image, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
	VkFormat FindDepthFormat() const;
	VkSampleCountFlagBits GetMaxUsableSampleCount() const;
	//*****************

	// The single time command block use a fence as such they will block the host
	SingleTimeCommandBuffer BeginSingleTimeCommands(VkCommandPool commandPool, VkQueue queue);
	void EndSingleTimeCommands(const SingleTimeCommandBuffer& commandBuffer);

	std::vector<const char*> GetRequiredExtensions() const;

	VkContext context;

	VkFormat swapChainImageFormat;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkImage> swapChainImages;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	uint32_t currentFrame = 0;
	AllocatedTexture depthImage;
	AllocatedTexture colorImage;

	std::unordered_map<EPipelineType, RenderPipeline*> renderPipelines;

	std::vector<Frame> renderFrames;

	std::vector<AllocatedBuffer> buffersPendingDelete;
	std::vector<AllocatedTexture> imagesPendingDelete;

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