#pragma once

#include "AssetManager/Asset.h"
#include "MeshData.h"
#include "Rendering/AbstractData.h"

#include <vector>

class Model : public Asset
{
public:
	virtual bool LoadAsset(const std::string& path) override;
	virtual void UnloadAsset() override;

	const MeshData& GetMeshData() const { return meshData; }
	const MeshRenderData& GetRenderData() const { return renderData; }

private:
	MeshData meshData;
	MeshRenderData renderData;
};