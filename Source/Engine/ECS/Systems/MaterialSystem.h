#pragma once

#include "AssetManager/LazyAssetPtr.h"
#include "ECS/Components/MaterialComponent.h"
#include "ECS/Entity.h"
#include "EngineName.h"
#include "SystemBase.h"
#include "Utilities/Color.h"

#include <glm/glm.hpp>
#include <unordered_map>

class Texture;

class MaterialSystem : public SystemBase
{
public:
	static MaterialSystem& Get();

	void UnInitialize() override;

	void CreateComponent(const Entity& entity, uint8_t pipeline, LazyAssetPtr textures[], int32_t texturesCount, const Color& color = Color::white);

	void RemoveComponent(const Entity& entity);

	bool TryGetMaterialData(const Entity& entity, uint32_t& outMaterialHandle) const;

	void SetColor(const Entity& entity, const Color& color);

private:
	void GetTextures(uint32_t componentID, std::vector<Texture*>& outTextures) const;

	void Allocate(int32_t size) override;
	void HandleEntityRemoved(const Entity& entity) override;
	void DestroyComponent(uint32_t componentID) override;

	MaterialComponent component;

	std::unordered_map<Entity, uint32_t> instances;
	std::unordered_multimap<uint32_t, LazyAssetPtr> cachedMultiTextures;
};