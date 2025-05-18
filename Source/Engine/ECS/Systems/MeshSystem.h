#pragma once

#include "AssetManager/LazyAssetPtr.h"
#include "ECS/Components/Component.h"
#include "ECS/Entity.h"
#include "SystemBase.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

struct MeshData;
struct MeshRenderData;

class MeshSystem : public SystemBase
{
public:
	static MeshSystem& Get();

	void UnInitialize() override;

	/// <summary>
	/// Creates a mesh component that holds the vertices data
	/// </summary>
	/// <param name="entity"> The entity parent </param>
	/// <param name="model"> The mesh (model) </param>
	/// <returns> component ID, can be used to access the data in the component</returns>
	uint32_t CreateComponent(const Entity& entity, LazyAssetPtr& model);

	/// <summary>
	/// Copies the mesh data to the outMeshData ptr (if null, nothing is copied)
	/// </summary>
	/// <param name="entity"> The entity </param>
	/// <param name="outMeshData"> The mesh data that will be returned </param>
	/// <returns> returns true if the entity has a mesh </returns>
	bool TryGetData(const Entity& entity, MeshData* outMeshData, MeshRenderData* outVulkanRenderData) const;

private:
	void Allocate(int32_t size) override;
	void DestroyComponent(uint32_t componentID) override;
	void HandleEntityRemoved(const Entity& entity) override;

	Component component;

	std::unordered_map<Entity, uint32_t> instances;
	std::unordered_map<uint32_t, LazyAssetPtr> cachedModels;
};