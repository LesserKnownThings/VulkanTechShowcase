#include "AssetManager.h"
#include "AssetPath.h"
#include "Utilities/FileHelper.h"

#include <string>
#include <vector>

AssetManager& AssetManager::Get()
{
	static AssetManager instance;
	return instance;
}

void AssetManager::UnInitialize()
{
	assets.clear();
}

void AssetManager::AddAssetRef(uint32_t handle)
{
	const std::string& name = nameRegistry.GetName(handle);
	auto it = assets.find(name);
	if (it != assets.end())
	{
		it->second.Increment();
	}
}

void AssetManager::AddEngineAssetRef(uint32_t handle)
{
	const std::string& name = engineNameRegistry.GetName(handle);
	auto it = engineAssets.find(name);
	if (it != engineAssets.end())
	{
		it->second.Increment();
	}
}

void AssetManager::ReleaseAsset(uint32_t handle)
{
	std::string name;
	if ((handle & ENGINE_ASSET_FLAG) != 0)
	{
		if (engineNameRegistry.TryGetName(handle, name))
		{
			auto it = engineAssets.find(name);
			if (it != engineAssets.end())
			{
				it->second.Decrement();
			}
		}
	}
	else
	{
		if (nameRegistry.TryGetName(handle, name))
		{
			auto it = assets.find(name);
			if (it != assets.end())
			{
				it->second.Decrement();
			}
		}
	}
}

void AssetManager::ImportAssets()
{
	static uint32_t handle = 0;
	ImportAssets("Data/Import", assets, nameRegistry, handle);
}

void AssetManager::ImportEngineAssets()
{
	static uint32_t handle = 0;
	ImportAssets("Data/Engine/Import", engineAssets, engineNameRegistry, handle, true);
}

void AssetManager::ImportAssets(const std::string& importDirectory, std::unordered_map<std::string, LazyAsset>& storage, AssetNameRegistry& registry, uint32_t& handle, bool isEngine)
{
	std::vector<fs::path> files;
	FileHelper::GetFilesFromDirectory(importDirectory, files, {}, "", true);

	for (int32_t i = 0; i < files.size(); ++i)
	{
		AssetPath path{ files[i] };
		const size_t dirPos = path.fullPath.find(importDirectory);

		if (dirPos != std::string::npos)
		{
			std::string assetPath = path.fullPath;
			assetPath.erase(dirPos, importDirectory.size() + 1);
			assetPath.erase(assetPath.size() - path.extension.size(), path.extension.size());
			storage.emplace(assetPath, LazyAsset{ path });

			uint32_t currentHandle = handle++;
			if (isEngine)
			{
				currentHandle |= ENGINE_ASSET_FLAG;
			}
			registry.Register(assetPath, currentHandle);
		}
	}
}
