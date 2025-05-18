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

	const std::vector<MeshData>& GetMeshes() const { return meshes; }
	const std::vector<MeshRenderData>& GetRenderData() const { return renderData; }

private:
	std::vector<MeshData> meshes;
	std::vector<MeshRenderData> renderData;
};