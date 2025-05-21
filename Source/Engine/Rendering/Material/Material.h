#pragma once

#include "Rendering/AbstractData.h"

#include <vector>

struct TextureSetKey
{
	std::vector<GenericHandle> views;
	std::vector<GenericHandle> samplers;

	bool operator==(const TextureSetKey&) const = default;
};

struct TextureSetKeyHash
{
	size_t operator()(const TextureSetKey& key) const noexcept
	{
		uint64_t h = 14695981039346656037ull;
		for (int32_t i = 0; i < key.views.size(); ++i)
		{
			h = (h ^ std::get<uintptr_t>(key.views[i])) * 1099511628211ull;
			h = (h ^ std::get<uintptr_t>(key.samplers[i])) * 1099511628211ull;
		}
		return static_cast<size_t>(h);
	}
};

struct Material
{
	EPipelineType pipeline;
	ERenderDataLoadState state;
	GenericHandle descriptorSet;
	TextureSetKey materialSlots;
};