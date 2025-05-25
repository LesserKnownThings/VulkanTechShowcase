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

struct AllocatedBuffer
{
	GenericHandle buffer;
	GenericHandle memory;
};

struct AllocatedImage
{
	GenericHandle image;
	GenericHandle view;
	GenericHandle sampler;
	GenericHandle memory;
};

struct MeshRenderData
{
	AllocatedBuffer vertex;
	AllocatedBuffer index;
	ERenderDataLoadState state = ERenderDataLoadState::Uninitialized;
};

struct TextureRenderData
{
	AllocatedImage image;
	ERenderDataLoadState state = ERenderDataLoadState::Uninitialized;
};
