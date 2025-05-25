#pragma once

#include "AbstractData.h"
#include "Material/Material.h"
#include "Utilities/Delegate.h"

#include <cstdint>
#include <entt/entity/registry.hpp>
#include <glm/glm.hpp>
#include <SDL3/SDL_video.h>
#include <string>
#include <unordered_map>
#include <vector>

class DescriptorRegistry;
class Texture;

struct DescriptorDataProvider;
struct DescriptorSetLayoutInfo;
struct MeshData;
struct MeshRenderData;
struct TextureData;
struct TextureRenderData;

DECLARE_DELEGATE_TwoParams(OnWindowResizeParams, float, float);

class RenderingInterface
{
public:
	virtual bool Initialize(int32_t inWidth = 0, int32_t inHeight = 0);
	virtual void UnInitialize();

	virtual void DrawFrame() = 0;
	virtual void EndFrame() = 0;

	virtual void DrawSingle(const std::vector<entt::entity>& entities, const entt::registry& registry) = 0;
	virtual void DrawLight(const std::vector<entt::entity>& entities, const entt::registry& registry) = 0;

	// Material descriptors
	virtual void AllocateMaterialDescriptorSet(EPipelineType pipeline, GenericHandle& outDescriptorSet) = 0;
	virtual void UpdateMaterialDescriptorSet(EPipelineType pipeline, const std::unordered_map<std::string, DescriptorDataProvider>& dataProviders) = 0;
	// *******************

	/// Global descriptors
	virtual void CreateGlobalUniformBuffers(std::array<AllocatedBuffer, MAX_FRAMES_IN_FLIGHT>& buffers) = 0;
	virtual void CreateLightBuffer(AllocatedBuffer& outBuffer) = 0;
	virtual void CreateGlobalDescriptorLayouts(DescriptorSetLayoutInfo& globalLayout, DescriptorSetLayoutInfo& lightLayout) = 0;
	virtual void AllocateGlobalDescriptorSet(const DescriptorSetLayoutInfo& layoutInfo, std::array<GenericHandle, MAX_FRAMES_IN_FLIGHT>& outDescriptorSets) = 0;
	virtual void AllocateLightDescriptorSet(const DescriptorSetLayoutInfo& layoutInfo, GenericHandle& outDescriptorSet) = 0;
	// *******************

	// Setters
	virtual void UpdateProjection(const glm::mat4& projection) = 0;
	virtual void UpdateView(const glm::mat4& view) = 0;
	//********

	virtual void CreateMeshVertexBuffer(const MeshData& meshData, MeshRenderData& outRenderData) = 0;
	virtual void CreateTextureBuffer(const TextureData& textureData, void* pixels, TextureRenderData& renderData) = 0;

	virtual void DestroyBuffer(GenericHandle buffer, GenericHandle bufferMemory) = 0;
	virtual void DestroyMeshVertexBuffer(const MeshRenderData& renderData) = 0;
	virtual void DestroyTextureBuffer(const TextureRenderData& renderData) = 0;

	SDL_Window* GetWindow() const { return window; }
	int32_t GetWindowWidth() const { return width; }
	int32_t GetWindowHeight() const { return height; }
	float GetAspectRatio() const { return aspectRatio; }

	DescriptorRegistry* GetDescriptorRegistry() const { return descriptorRegistry; }

	OnWindowResizeParams onWindowResizeParams;

protected:
	virtual void HandleWindowResized();
	virtual void HandleWindowMinimized() = 0;
	void CreateDescriptorRegistry();
	void RenderTasks();

	int32_t width = 960;
	int32_t height = 540;

	float aspectRatio = 1.0f;

	SDL_Window* window = nullptr;
	const std::string applicationName = "Tech Showcase";

	// Subsystems
	DescriptorRegistry* descriptorRegistry = nullptr;
};