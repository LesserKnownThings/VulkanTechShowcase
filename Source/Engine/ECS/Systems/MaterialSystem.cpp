#include "MaterialSystem.h"
#include "AssetManager/AssetManager.h"
#include "AssetManager/Texture/Texture.h"
#include "ECS/Components/Components.h"
#include "Engine.h"
#include "Rendering/AbstractData.h"
#include "Rendering/Descriptors/DescriptorInfo.h"
#include "Rendering/RenderingInterface.h"

#include <glm/glm.hpp>

void MaterialSystem::ReleaseResources()
{
	for (const auto& it : materialInstances)
	{
		for (const MaterialDescriptorBindingResource& resource : it.second.key.resources)
		{
			AssetManager::Get().ReleaseAsset(resource.textureAssetHandle);
		}
	}
}

Material MaterialSystem::CreatePBRMaterial(std::optional<uint32_t> albedo)
{
	AssetManager& assetManager = AssetManager::Get();

	// Load the engine defaults if user did not provide a texture
	if (albedo == std::nullopt)
	{
		uint32_t handles[1];
		assetManager.QueryEngineAssets(handles, "Textures\\Albedo");
		albedo = handles[0];
	}

	MaterialInstanceKey key
	{
		EPipelineType::PBR,
		{
			MaterialDescriptorBindingResource {"abledo", albedo.value()}
		}
	};

	auto it = materialHandles.find(key);
	if (it != materialHandles.end())
	{
		return Material{ key.pipeline, it->second };
	}

	uint32_t value = materialHandleTracker++;
	materialHandles[key] = value;

	MaterialInstance instance{};
	instance.key = key;
	instance.state = ERenderDataLoadState::Ready;

	RenderingInterface* renderingInterface = GameEngine->GetRenderingSystem();

	renderingInterface->AllocateMaterialDescriptorSet(key.pipeline, instance.descriptorSet);
	// Need to update the texture in the material after creating the sets

	const Texture* texture = assetManager.LoadAsset<Texture>(albedo.value());
	const TextureRenderData& renderData = texture->GetRenderData();

	DescriptorDataProvider albedoProvider{};
	// TODO maybe have a better way to set the descriptor set index?
	albedoProvider.descriptorSet = instance.descriptorSet;
	albedoProvider.providerType = EDescriptorDataProviderType::Texture;
	albedoProvider.texture = renderData.texture;

	std::unordered_map<std::string, DescriptorDataProvider> providers =
	{
		std::make_pair("albedo", albedoProvider)
	};
	renderingInterface->UpdateMaterialDescriptorSet(instance.key.pipeline, providers);

	materialInstances[value] = instance;

	return Material{ key.pipeline, value };
}

bool MaterialSystem::TryGetMaterialInstance(uint32_t handle, MaterialInstance& outInstance) const
{
	auto it = materialInstances.find(handle);
	if (it != materialInstances.end())
	{
		outInstance = it->second;
		return true;
	}
	return false;
}
