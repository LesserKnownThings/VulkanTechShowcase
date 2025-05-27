#pragma once

#include "Rendering/AbstractData.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>

constexpr uint32_t MATRICES_DESCRIPTOR_FLAG = 1 << 0;
constexpr uint32_t LIGHT_DESCRIPTOR_FLAG = 1 << 1;

enum class EDescriptorOwner : uint8_t
{
	None,
	View,
	Material
};

/// <summary>
/// Abstract struct that holds the descriptor layout
/// </summary>
struct DescriptorSetLayoutInfo
{
	GenericHandle layout;
	uint32_t setIndex = 0;
	EDescriptorOwner owner = EDescriptorOwner::None;
};

/// <summary>
/// Abstract struct used to create descriptor writes
/// The write is based on the binding and semantic
/// This should be created at binding creation of the set
/// </summary>
struct DescriptorBindingInfo
{
	EDescriptorOwner owner;
	uint32_t set = 0;
	uint32_t binding = 0;
	size_t type;
	size_t stage;
	std::string semantic;
};

enum class EDescriptorDataProviderType
{
	None,
	Texture,
	Buffer,
	All
};

struct DescriptorSetDataProviderBuffer
{
	uint32_t offset = 0;
	uint32_t range = 0;
	AllocatedBuffer buffer;
};

/// <summary>
/// Abstrac struct used to set the texture / buffer data in the set writes
/// You can choose if you want to send and texture a buffer or both wit the provider type
/// The buffer also allows you to provide an offset and a range in the buffer
/// </summary>
struct DescriptorDataProvider
{
	EDescriptorDataProviderType providerType;
	GenericHandle descriptorSet;

	DescriptorSetDataProviderBuffer dataProviderBuffer;
	AllocatedTexture texture;
};