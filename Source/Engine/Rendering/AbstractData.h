#pragma once

#include <cstdint>
#include <variant>

using GenericHandle = std::variant<uint32_t, uintptr_t>;

constexpr int32_t MAX_FRAMES_IN_FLIGHT = 2;

enum class ERenderDataLoadState : uint8_t
{
	Uninitialized,
	Loading,
	Ready,
	Failed
};

enum class EPipelineType : uint8_t
{
	Unlit,
	PBR,
	UI,
	Skybox,
	Debug
};

/// <summary>
/// I'm abstracting this for both open GL and Vulkan
/// One of the memory handles can be used for the vao on open GL
/// </summary>
struct MeshRenderData
{
	GenericHandle vertexBuffer;
	GenericHandle indexBuffer;

	GenericHandle vertexMemory;
	GenericHandle indexMemory;

	ERenderDataLoadState state = ERenderDataLoadState::Uninitialized;
};

struct TextureRenderData
{
	GenericHandle textureImage;
	GenericHandle textureImageMemory;
	GenericHandle imageView;
	GenericHandle imageSampler;

	ERenderDataLoadState state = ERenderDataLoadState::Uninitialized;
};