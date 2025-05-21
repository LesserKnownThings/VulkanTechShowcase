#include "Model.h"
#include "Engine.h"
#include "Rendering/RenderingInterface.h"
#include "Utilities/MeshImporter.h"

bool Model::LoadAsset(const std::string& path)
{
	MeshImporter::ImportModel(path, meshes);
	RenderingInterface* renderingInterface = GameEngine->GetRenderingSystem();

	const int32_t meshesCount = meshes.size();
	renderData.resize(meshesCount);
	for (int32_t i = 0; i < meshesCount; ++i)
	{
		const MeshData& meshData = meshes[i];
		MeshRenderData& mRenderData = renderData[i];
		mRenderData.state = ERenderDataLoadState::Loading;
		renderingInterface->CreateMeshVertexBuffer(meshData, mRenderData);
	}

	return true;
}

void Model::UnloadAsset()
{
	for (MeshRenderData& meshRenderData : renderData)
	{
		GameEngine->GetRenderingSystem()->DestroyMeshVertexBuffer(meshRenderData);
	}
}
