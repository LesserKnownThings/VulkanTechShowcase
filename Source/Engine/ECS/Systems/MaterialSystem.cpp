#include "MaterialSystem.h"
#include "AssetManager/AssetManager.h"
#include "AssetManager/Model/MeshData.h"
#include "AssetManager/Texture/Texture.h"
#include "ECS/Components/Components.h"
#include "Engine.h"
#include "Rendering/AbstractData.h"
#include "Rendering/Descriptors/DescriptorInfo.h"
#include "Rendering/Material/MaterialSemantics.h"
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

Material MaterialSystem::CreatePBRMaterial(std::optional<uint32_t> albedo, bool makeUnique)
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
			MaterialDescriptorBindingResource {MaterialSemantics::Albedo, albedo.value()}
		}
	};

	// We want to create a unique material so no need to try and find an existing layout
	if (!makeUnique)
	{
		auto it = materialHandles.find(key);
		if (it != materialHandles.end())
		{
			return Material{ static_cast<uint8_t>(key.pipeline), it->second };
		}
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

	std::unordered_map<EngineName, DescriptorDataProvider> providers =
	{
		std::make_pair(MaterialSemantics::Albedo, albedoProvider)
	};
	renderingInterface->UpdateMaterialDescriptorSet(instance.key.pipeline, providers);

	materialInstances[value] = instance;

	return Material{ static_cast<uint8_t>(key.pipeline), value };
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

void MaterialSystem::SetTextures(uint32_t handle, const std::vector<MaterialDescriptorBindingResource>& resources)
{
	auto it = materialInstances.find(handle);
	if (it != materialInstances.end())
	{
		MaterialInstance& instance = it->second;
		AssetManager& assetManager = AssetManager::Get();

		std::unordered_map<EngineName, DescriptorDataProvider> providers{};

		for (const MaterialDescriptorBindingResource& resource : resources)
		{
			const Texture* texture = assetManager.LoadAsset<Texture>(resource.textureAssetHandle);
			const TextureRenderData& renderData = texture->GetRenderData();

			DescriptorDataProvider provider{};
			// TODO maybe have a better way to set the descriptor set index?
			provider.descriptorSet = instance.descriptorSet;
			provider.providerType = EDescriptorDataProviderType::Texture;
			provider.texture = renderData.texture;

			providers[resource.semantic] = provider;

			// Update the texture ref and release the previous texture
			// TODO A material can have max 4 - 6 resources, so this for is not that bad, but maybe test with a map?
			for (MaterialDescriptorBindingResource& currentResource : instance.key.resources)
			{
				if (currentResource.semantic == resource.semantic)
				{
					assetManager.ReleaseAsset(currentResource.textureAssetHandle);
					currentResource.textureAssetHandle = resource.textureAssetHandle;
					break;
				}
			}
		}

		RenderingInterface* renderingInterface = GameEngine->GetRenderingSystem();
		renderingInterface->UpdateMaterialDescriptorSet(instance.key.pipeline, providers);
	}
}
