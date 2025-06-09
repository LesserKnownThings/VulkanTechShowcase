#pragma once

#include "AbstractData.h"
#include "EngineName.h"
#include "Material/Material.h"
#include "Utilities/Delegate.h"

#include <cstdint>
#include <glm/glm.hpp>
#include <SDL3/SDL_video.h>
#include <string>
#include <unordered_map>
#include <vector>

class DescriptorRegistry;
class MaterialSystem;
class Texture;

struct DescriptorDataProvider;
struct DescriptorSetLayoutInfo;
struct LightInstance;
struct MeshData;
struct MeshRenderData;
struct TextureData;
struct TextureRenderData;
struct View;

DECLARE_DELEGATE(OnRenderFrameReset);
DECLARE_DELEGATE_TwoParams(OnWindowResizeParams, float, float);

constexpr uint32_t SHADOW_MAP_SIZE = 4096; // ULTRA

enum class EBufferType
{
	Uniform,
	Storage
};

enum class EDescriptorSetType
{
	Uniform,
	UniformDynamic,
	Storage,
	StorageDynamic
};

class RenderingInterface
{
public:
	virtual bool Initialize(int32_t inWidth = 0, int32_t inHeight = 0);
	virtual void UnInitialize();

	virtual void DrawFrame() = 0;
	virtual void EndFrame() = 0;	

	// Descriptor Sets
	virtual bool TryGetDescriptorLayoutForOwner(EPipelineType pipeline, EDescriptorOwner owner, DescriptorSetLayoutInfo& outLayoutInfo) = 0;
	virtual void UpdateDescriptorSet(EPipelineType pipeline, const std::unordered_map<EngineName, DescriptorDataProvider>& dataProviders) = 0;
	// *******************

	/// Global descriptors
	virtual void CreateBuffer(EBufferType bufferType, uint32_t size, AllocatedBuffer& outBuffer) = 0;
	virtual void CreateGlobalDescriptorLayouts(DescriptorSetLayoutInfo& cameraMatricesLayout, DescriptorSetLayoutInfo& lightLayout, DescriptorSetLayoutInfo& animationLayout, DescriptorSetLayoutInfo& shadowLayout) = 0;
	virtual void CreateDescriptorSet(const DescriptorSetLayoutInfo& layoutInfo, GenericHandle& outDescriptorSet) = 0;

	virtual void UpdateDescriptorSet(EDescriptorSetType type, GenericHandle descriptorSet, AllocatedBuffer buffer, uint32_t binding = 0) = 0;
	virtual void UpdateDynamicDescriptorSet(EDescriptorSetType type, GenericHandle descriptorSet, AllocatedBuffer buffer, uint32_t binding, uint32_t offset, uint32_t range) = 0;
	virtual void DestroyDescriptorSetLayout(const DescriptorSetLayoutInfo& layoutInfo) = 0;
	// *******************

	// Buffer manips
	virtual void CreateMeshVertexBuffer(const MeshData& meshData, MeshRenderData& outRenderData) = 0;
	virtual void CreateTextureBuffer(const TextureData& textureData, void* pixels, TextureRenderData& renderData) = 0;

	virtual void UpdateBuffer(AllocatedBuffer buffer, uint32_t offset, uint32_t range, void* dataToCopy) = 0;

	virtual void DestroyBuffer(AllocatedBuffer buffer) = 0;
	virtual void DestroyTexture(AllocatedTexture texture) = 0;
	// ************

	SDL_Window* GetWindow() const { return window; }
	int32_t GetWindowWidth() const { return width; }
	int32_t GetWindowHeight() const { return height; }
	float GetAspectRatio() const { return aspectRatio; }

	DescriptorRegistry* GetDescriptorRegistry() const { return descriptorRegistry; }
	MaterialSystem* GetMaterialSystem() const { return materialSystem; }

	OnWindowResizeParams onWindowResizeParams;
	OnRenderFrameReset onRenderFrameReset;

	static uint32_t MIN_UNIFORM_ALIGNMENT;

protected:
	virtual void DrawShadows(const View& view) = 0;
	virtual void DrawSingle(const View& view) = 0;

	virtual void UpdateProjection(const glm::mat4& projection) = 0;
	virtual void UpdateView(const glm::mat4& view) = 0;

	virtual void HandleWindowResized();
	virtual void HandleWindowMinimized() = 0;
	void CreateDescriptorRegistry();

	int32_t width = 960;
	int32_t height = 540;

	float aspectRatio = 1.0f;

	SDL_Window* window = nullptr;
	const std::string applicationName = "Tech Showcase";

	// Subsystems
	DescriptorRegistry* descriptorRegistry = nullptr;
	MaterialSystem* materialSystem = nullptr;	
};