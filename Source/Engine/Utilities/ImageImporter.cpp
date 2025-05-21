#include "ImageImporter.h"
#include "AssetManager/Texture/TextureData.h"
#include "Engine.h"
#include "Rendering/RenderingInterface.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

bool ImageImporter::ImportTexture(const std::string& path, TextureData& outData, TextureRenderData& outTextureRenderData)
{
	int32_t& width = outData.width;
	int32_t& height = outData.height;
	int32_t& channels = outData.channels;

	if (stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha))
	{
		outData.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
		RenderingInterface* renderingInterface = GameEngine->GetRenderingSystem();
		renderingInterface->CreateImageBuffer(outData, pixels, outTextureRenderData);
		return true;
	}
	return false;
}
