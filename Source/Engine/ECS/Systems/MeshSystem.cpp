#include "MeshSystem.h"
#include "AssetManager/Model/MeshData.h"
#include "AssetManager/Model/Model.h"
#include "ECS/Entity.h"
#include "Rendering/AbstractData.h"

#include <iostream>
#include <glm/glm.hpp>

MeshSystem& MeshSystem::Get()
{
	static MeshSystem instance;
	return instance;
}

void MeshSystem::UnInitialize()
{
	instances.clear();
	cachedModels.clear();
	free(component.buffer);
}

void MeshSystem::HandleEntityRemoved(const Entity& entity)
{
	auto it = instances.find(entity.id);
	if (it != instances.end())
	{
		const uint32_t cID = it->second;
		component.isAlive[cID] = false;
		pendingDeletion.push_back(cID);
	}
}

uint32_t MeshSystem::CreateComponent(const Entity& entity, LazyAssetPtr& model)
{
	const uint32_t cID = component.instances;

	if (component.allocatedInstances <= cID)
	{
		Allocate(component.allocatedInstances * 2);
	}

	instances[entity.id] = cID;
	component.instances++;

	component.entity[cID] = entity;
	component.isAlive[cID] = true;

	cachedModels[cID] = std::move(model);

	return cID;
}

bool MeshSystem::TryGetData(const Entity& entity, MeshData* outMeshData, MeshRenderData* outVulkanRenderData) const
{
	auto it = instances.find(entity);
	if (it != instances.end())
	{
		auto modelIT = cachedModels.find(it->second);
		if (modelIT != cachedModels.end())
		{
			if (const Model* model = modelIT->second.Get<Model>())
			{
				if (outMeshData != nullptr)
				{
					const std::vector<MeshData>& meshes = model->GetMeshes();

					// Only supporting 1 mesh
					memcpy(outMeshData, &meshes[0], sizeof(int32_t) * 2);

					outMeshData->indices.resize(meshes[0].indicesCount);
					memcpy(&outMeshData->indices[0], &meshes[0].indices[0], sizeof(int32_t) * meshes[0].indicesCount);

					outMeshData->vertices.resize(meshes[0].verticesCount);
					memcpy(&outMeshData->vertices[0], &meshes[0].vertices[0], sizeof(Vertex) * meshes[0].verticesCount);
				}

				if (outVulkanRenderData != nullptr)
				{
					const std::vector<MeshRenderData>& vkRenderData = model->GetRenderData();
					*outVulkanRenderData = vkRenderData[0];
				}
				return true;
			}
		}
	}
	return false;
}

void MeshSystem::DestroyComponent(uint32_t componentID)
{
	const uint32_t last = component.instances - 1;
	const Entity currentEntity = component.entity[componentID];
	const Entity lastEntity = component.entity[last];

	component.entity[componentID] = component.entity[last];
	component.isAlive[componentID] = component.isAlive[last];

	cachedModels[componentID] = std::move(cachedModels[last]);
	cachedModels.erase(last);

	instances[lastEntity.id] = componentID;
	instances.erase(currentEntity.id);
	component.instances--;
}

void MeshSystem::Allocate(int32_t size)
{
	Component newComponent;

	const int32_t sizeToAllocate = size * (sizeof(Entity) + sizeof(uint8_t));
	newComponent.buffer = malloc(sizeToAllocate);

	newComponent.instances = component.instances;
	newComponent.allocatedInstances = size;

	newComponent.entity = (Entity*)(newComponent.buffer);
	newComponent.isAlive = (uint8_t*)(newComponent.entity + size);

	memcpy(newComponent.entity, component.entity, component.instances * sizeof(Entity));
	memcpy(newComponent.isAlive, component.isAlive, component.instances * sizeof(uint8_t));

	free(component.buffer);
	component = newComponent;
}
