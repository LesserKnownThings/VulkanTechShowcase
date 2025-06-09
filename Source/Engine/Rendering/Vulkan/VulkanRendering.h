#pragma once

#include "Frame.h"
#include "Rendering/AbstractData.h"
#include "Rendering/RenderingInterface.h"
#include "VkContext.h"

#include <array>
#include <SDL3/SDL_video.h>
#include <string>
#include <vector>
#include <vma/vk_mem_alloc.h>
#include <volk.h>
#include <unordered_map>

class RenderPipeline;

#if WITH_EDITOR
class EditorUI;
#endif

struct MeshData;
struct MeshRenderData;

enum class EKeyPressType : uint8_t;

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

struct SwapChainData
{
	void Resize(uint32_t size)
	{
		swapChainFramebuffers.resize(size);
		images.resize(size);
		views.resize(size);
		depths.resize(size);
		colors.resize(size);
	}

	std::vector<VkFramebuffer> swapChainFramebuffers;

	std::vector<VkImage> images;
	std::vector<VkImageView> views;

	std::vector<AllocatedTexture> depths;
	std::vector<AllocatedTexture> colors;
};

struct ShadowMapData
{
	VkFramebuffer shadowMappingFB = VK_NULL_HANDLE;
	AllocatedTexture shadowDepthTexture;
};

class VulkanRendering : public RenderingInterface
{
public:
	bool Initialize(int32_t inWidth = 0, int32_t inHeight = 0) override;
	void UnInitialize() override;

	void DrawFrame() override;
	void EndFrame() override;

	const VkContext& GetContext() const { return context; }
	VkCommandBuffer GetCurrentCommandBuffer() const;

	// Material descriptors
	bool TryGetDescriptorLayoutForOwner(EPipelineType pipeline, EDescriptorOwner owner, DescriptorSetLayoutInfo& outLayoutInfo) override;
	void UpdateDescriptorSet(EPipelineType pipeline, const std::unordered_map<EngineName, DescriptorDataProvider>& dataProviders) override;
	// *******************

	/// Global descriptors
	void CreateBuffer(EBufferType bufferType, uint32_t size, AllocatedBuffer& outBuffer) override;
	void CreateGlobalDescriptorLayouts(DescriptorSetLayoutInfo& cameraMatricesLayout, DescriptorSetLayoutInfo& lightLayout, DescriptorSetLayoutInfo& animationLayout, DescriptorSetLayoutInfo& shadowLayout) override;
	void CreateDescriptorSet(const DescriptorSetLayoutInfo& layoutInfo, GenericHandle& outDescriptorSet) override;
	void UpdateDescriptorSet(EDescriptorSetType type, GenericHandle descriptorSet, AllocatedBuffer buffer, uint32_t binding = 0) override;
	void UpdateDynamicDescriptorSet(EDescriptorSetType type, GenericHandle descriptorSet, AllocatedBuffer buffer, uint32_t binding, uint32_t offset, uint32_t range) override;
	void DestroyDescriptorSetLayout(const DescriptorSetLayoutInfo& layoutInfo) override;
	// *******************

	// Buffer manips
	void CreateMeshVertexBuffer(const MeshData& meshData, MeshRenderData& outRenderData) override;
	void CreateTextureBuffer(const TextureData& textureData, void* pixels, TextureRenderData& renderData) override;

	void UpdateBuffer(AllocatedBuffer buffer, uint32_t offset, uint32_t range, void* dataToCopy) override;

	void DestroyBuffer(AllocatedBuffer buffer) override;
	void DestroyTexture(AllocatedTexture texture) override;
	// ************

protected:
	void DrawShadows(const View& view) override;
	void DrawSingle(const View& view) override;

	void UpdateProjection(const glm::mat4& projection) override;
	void UpdateView(const glm::mat4& view) override;

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
	void CreateShadowPassImage();
	bool CreateRenderPass();
	void CreateShadowPass();
	bool CreateFrameBuffers();
	void CreateShadowMappingFB();
	bool CreateCommandPools();
	bool CreateDescriptorPool();

	void CreateRenderFrames();
	void CreateRenderPipelines();
	void SetupDebugMessenger();

	void CleanupSwapChain();
	void CleanupPendingDestroyBuffers();
	void RecreateSwapChain();

	void RecordCommandBuffer(uint32_t imageIndex);
	void ShadowRenderPass(const View& view);
	void DebugShadowPass(uint32_t imageIndex, const View& view);
	void DrawRenderPass(uint32_t imageIndex, const View& view);

	void DrawShadowDebugQuad(const View& view);

	void CreateColorResources();
	void CreateDepthResources();

	bool IsDeviceSuitable(VkPhysicalDevice device) const;
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;

	void CreateVertexBuffer(const std::vector<glm::vec3>& data, AllocatedBuffer& outBuffer);

	int32_t RateDeviceSuitability(VkPhysicalDevice device) const;
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) const;
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	// Helper functions
	void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VmaAllocationCreateFlags memoryFlags, VkImage& outImage, VmaAllocation& outMemory);

	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

	void TransitionShadowLayoutToFragment(VkCommandBuffer commandBuffer);
	void TransitionShadowLayoutToGeometry(VkCommandBuffer commandBuffer);

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

	void OnKeyPressed(uint32_t key, EKeyPressType pressType);

	std::vector<const char*> GetRequiredExtensions() const;

	VkContext context;

	VkFormat swapChainImageFormat;
	SwapChainData swapChainData;

	uint32_t currentFrame = 0;

	// Shadows
	std::array<ShadowMapData, MAX_FRAMES_IN_FLIGHT> shadowMapData;

	// Need to track the shadow image since it will transfered between layouts to be sampled
	VkImageLayout currentShadowImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// *******

	std::unordered_map<EPipelineType, RenderPipeline*> renderPipelines;

	std::vector<Frame> renderFrames;
	std::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> additiveFrameBuffers;

	std::vector<AllocatedBuffer> buffersPendingDelete;
	std::vector<AllocatedTexture> imagesPendingDelete;

	std::vector<const char*> instanceExtensions =
	{
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME
	};

	std::vector<const char*> deviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME		
	};

	// **** Editor *****
	const std::vector<const char*> validationLayers =
	{
		"VK_LAYER_KHRONOS_validation"
	};

#if WITH_EDITOR
	EditorUI* editorUI = nullptr;
#endif

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif
};