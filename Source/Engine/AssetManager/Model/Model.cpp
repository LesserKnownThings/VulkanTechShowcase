#include "Model.h"
#include "ECS/Systems/MaterialSystem.h"
#include "Engine.h"
#include "Rendering/RenderingInterface.h"
#include "Utilities/MeshImporter.h"

bool Model::LoadAsset(const std::string& path)
{
	MeshImporter::ImportModel(path, meshData);
	RenderingInterface* renderingInterface = GameEngine->GetRenderingSystem();

	renderingInterface->CreateMeshVertexBuffer(meshData, renderData);
	return true;
}

void Model::UnloadAsset()
{
	RenderingInterface* renderingInterface = GameEngine->GetRenderingSystem();
	renderingInterface->DestroyBuffer(renderData.vertex);
	renderingInterface->DestroyBuffer(renderData.index);
}
