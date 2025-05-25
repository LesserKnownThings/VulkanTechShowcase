#pragma once

#include "Rendering/AbstractData.h"
#include "Rendering/Descriptors/DescriptorInfo.h"

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

template <typename T>
inline void HashCombine(std::size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

struct MaterialDescriptorBindingResource
{
	std::string semantic;
	uint32_t imageHandle;

	bool operator==(const MaterialDescriptorBindingResource& other) const
	{
		return semantic == other.semantic &&
			imageHandle == other.imageHandle;
	}
};

template <>
struct std::hash<MaterialDescriptorBindingResource>
{
	std::size_t operator()(const MaterialDescriptorBindingResource& resource) const noexcept
	{
		std::size_t seed = 0;
		HashCombine(seed, resource.semantic);
		HashCombine(seed, resource.imageHandle);
		return seed;
	}
};

struct MaterialInstanceKey
{
	EPipelineType pipeline;
	std::vector<MaterialDescriptorBindingResource> resources;

	mutable std::size_t cachedHash = 0;

	bool operator==(const MaterialInstanceKey& other) const
	{
		return pipeline == other.pipeline && resources == other.resources;
	}

	std::size_t GetHash() const
	{
		if (cachedHash == 0)
		{
			std::size_t seed = 0;
			HashCombine(seed, static_cast<std::underlying_type_t<EPipelineType>>(pipeline));
			for (const auto& resource : resources) {
				HashCombine(seed, resource);
			}
			cachedHash = seed;
		}
		return cachedHash;
	}
};

template <>
struct std::hash<MaterialInstanceKey>
{
	std::size_t operator()(const MaterialInstanceKey& key) const noexcept
	{
		return key.GetHash();
	}
};

struct MaterialInstance
{
	MaterialInstanceKey key;
	GenericHandle descriptorSet;
	ERenderDataLoadState state;
};