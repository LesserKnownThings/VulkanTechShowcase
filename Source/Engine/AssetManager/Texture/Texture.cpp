#include "Texture.h"
#include "Engine.h"
#include "Rendering/RenderingInterface.h"
#include "Utilities/ImageImporter.h"

bool Texture::LoadAsset(const std::string& path)
{
	renderData.state = ERenderDataLoadState::Loading;
	return ImageImporter::ImportTexture(path, data, renderData);
}

void Texture::UnloadAsset()
{
	GameEngine->GetRenderingSystem()->DestroyTextureBuffer(renderData);
}
