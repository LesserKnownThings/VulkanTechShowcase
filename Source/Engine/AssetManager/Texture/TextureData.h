#pragma once

#include <cstdint>

struct TextureData
{
	int32_t width = 0;
	int32_t height = 0;
	int32_t channels = 0;
	uint32_t mipLevels = 0;
};