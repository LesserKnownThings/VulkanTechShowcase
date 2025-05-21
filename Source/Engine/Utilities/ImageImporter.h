#pragma once

#include <string>

struct TextureData;
struct TextureRenderData;

class ImageImporter
{
public:
	static bool ImportTexture(const std::string& path, TextureData& outData, TextureRenderData& outTextureRenderData);
};