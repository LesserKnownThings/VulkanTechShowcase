#pragma once

#include "AssetManager/Asset.h"
#include "Rendering/AbstractData.h"
#include "TextureData.h"

class Texture : public Asset
{
public:
	virtual bool LoadAsset(const std::string& path) override;
	virtual void UnloadAsset() override;

	const TextureData& GetData() const { return data; }
	const TextureRenderData& GetRenderData() const { return renderData; }
	bool IsReady() const { return renderData.state == ERenderDataLoadState::Ready; }

private:
	TextureData data;
	TextureRenderData renderData;
};