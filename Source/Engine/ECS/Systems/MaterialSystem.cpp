#include "MaterialSystem.h"
#include "AssetManager/Texture/Texture.h"
#include "Engine.h"
#include "Rendering/Material/Material.h"
#include "Rendering/RenderingInterface.h"

#include <thread>

MaterialSystem& MaterialSystem::Get()
{
	static MaterialSystem instance;
	return instance;
}

void MaterialSystem::CreateComponent(const Entity& entity, uint8_t pipeline, LazyAssetPtr textures[], int32_t texturesCount, const Color& color)
{
	const uint32_t cID = component.instances;
	if (cID >= component.allocatedInstances)
	{
		Allocate(component.allocatedInstances * 2);
	}

	component.instances++;

	component.color[cID] = color.GetVectorAlpha();
	component.entity[cID] = entity;
	component.pipeline[cID] = pipeline;
	component.state[cID] = EComponentState::Ready;

	for (int32_t i = 0; i < texturesCount; ++i)
	{
		cachedMultiTextures.emplace(cID, std::move(textures[i]));
	}

	std::vector<Texture*> loadedTextures;
	GetTextures(cID, loadedTextures);

	const int32_t loadedTexturesSize = loadedTextures.size();
	TextureSetKey slots{};
	slots.samplers.resize(loadedTexturesSize);
	slots.views.resize(loadedTexturesSize);

	for (int32_t i = 0; i < loadedTexturesSize; ++i)
	{
		Texture* texture = loadedTextures[i];
		const TextureRenderData& renderData = texture->GetRenderData();
		slots.samplers[i] = renderData.imageSampler;
		slots.views[i] = renderData.imageView;
	}

	component.materialHandle[cID] = GameEngine->GetRenderingSystem()->GetOrCreateMaterialHandle(pipeline, slots);

	instances.emplace(entity, cID);
}

void MaterialSystem::RemoveComponent(const Entity& entity)
{
	auto it = instances.find(entity);
	if (it != instances.end())
	{
		DestroyComponent(it->second);
	}
}

bool MaterialSystem::TryGetMaterialData(const Entity& entity, uint32_t& outMaterialHandle) const
{
	auto it = instances.find(entity);
	if (it != instances.end())
	{
		const uint32_t& cID = it->second;

		if (component.state[cID] != EComponentState::Ready)
		{
			return false;
		}

		outMaterialHandle = component.materialHandle[cID];
		return true;
	}

	return false;
}

void MaterialSystem::GetTextures(uint32_t componentID, std::vector<Texture*>& outTextures) const
{
	auto range = cachedMultiTextures.equal_range(componentID);
	for (auto& textureIt = range.first; textureIt != range.second; ++textureIt)
	{
		if (Texture* texture = textureIt->second.Get<Texture>())
		{
			outTextures.push_back(texture);
		}
	}
}

void MaterialSystem::SetColor(const Entity& entity, const Color& color)
{
	auto it = instances.find(entity);
	if (it != instances.end())
	{
		component.color[it->second] = color.GetVectorAlpha();
	}
}

void MaterialSystem::UnInitialize()
{
	free(component.buffer);
	cachedMultiTextures.clear();
	instances.clear();
}

void MaterialSystem::Allocate(int32_t size)
{
	MaterialComponent cpy;
	cpy.instances = component.instances;
	cpy.allocatedInstances = size;

	const int32_t bufferSize = size * (sizeof(Entity) + sizeof(uint32_t) + sizeof(EComponentState) + sizeof(uint8_t));
	cpy.buffer = malloc(bufferSize);

	cpy.color = (glm::vec4*)cpy.buffer;
	cpy.entity = (Entity*)(size + cpy.color);
	cpy.materialHandle = (uint32_t*)(size + cpy.entity);
	cpy.pipeline = (uint8_t*)(size + cpy.materialHandle);
	cpy.state = (EComponentState*)(cpy.pipeline + size);

	memcpy(cpy.color, component.color, sizeof(glm::vec4) * component.instances);
	memcpy(cpy.entity, component.entity, sizeof(Entity) * component.instances);
	memcpy(cpy.materialHandle, component.materialHandle, sizeof(uint32_t) * component.instances);
	memcpy(cpy.pipeline, component.pipeline, sizeof(uint8_t) * component.instances);
	memcpy(cpy.state, component.state, sizeof(EComponentState) * component.instances);

	free(component.buffer);
	component = cpy;
}

void MaterialSystem::HandleEntityRemoved(const Entity& entity)
{
	auto it = instances.find(entity.id);
	if (it != instances.end())
	{
		component.state[it->second] = EComponentState::Dead;
		pendingDeletion.push_back(it->second);
	}
}

void MaterialSystem::DestroyComponent(uint32_t componentID)
{
	const uint32_t last = component.instances - 1;
	const Entity currentEntity = component.entity[componentID];
	const Entity lastEntity = component.entity[last];

	component.color[componentID] = component.color[last];
	component.entity[componentID] = component.entity[last];
	component.materialHandle[componentID] = component.materialHandle[last];
	component.pipeline[componentID] = component.pipeline[last];
	component.state[componentID] = component.state[last];

	std::vector<LazyAssetPtr> valuesToMove;
	for (auto it = cachedMultiTextures.lower_bound(last); it != cachedMultiTextures.upper_bound(last); ++it)
	{
		valuesToMove.push_back(std::move(it->second));
	}

	cachedMultiTextures.erase(componentID);
	cachedMultiTextures.erase(last);

	for (LazyAssetPtr& assetPtr : valuesToMove)
	{
		cachedMultiTextures.emplace(componentID, std::move(assetPtr));
	}

	valuesToMove.clear();

	instances[lastEntity.id] = componentID;
	instances.erase(currentEntity.id);
	component.instances--;
}
