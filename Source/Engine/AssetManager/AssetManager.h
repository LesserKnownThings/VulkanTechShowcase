#pragma once

#include "LazyAsset.h"

#include <cassert>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

constexpr uint32_t ENGINE_ASSET_FLAG = 1u << 31;

struct AssetNameRegistry
{
	std::unordered_map<std::string, uint32_t> nameToHandle;
	std::unordered_map<uint32_t, std::string> handleToName;

	void Register(const std::string& name, uint32_t handle)
	{
		nameToHandle[name] = handle;
		handleToName[handle] = name;
	}

	uint32_t GetHandle(const std::string& name) const
	{
		auto it = nameToHandle.find(name);
		assert(it != nameToHandle.end());
		return it->second;
	}

	std::string GetName(uint32_t handle) const
	{
		auto it = handleToName.find(handle);
		assert(it != handleToName.end());
		return it->second;
	}

	bool TryGetName(uint32_t handle, std::string& outName) const
	{
		auto it = handleToName.find(handle);
		if (it != handleToName.end())
		{
			outName = it->second;
			return true;
		}
		return false;
	}
};

class AssetManager
{
public:
	static AssetManager& Get();

	void UnInitialize();
		
	void ReleaseAsset(uint32_t handle);

	template <typename... Keys>
	void QueryAssets(uint32_t assetHandles[], Keys... keys);

	template<typename... Keys>
	void QueryEngineAssets(uint32_t assetHandles[], Keys... keys);

	template<typename T>
	T* LoadAsset(uint32_t handle);

private:
	AssetManager() = default;

	void AddAssetRef(uint32_t handle);
	void AddEngineAssetRef(uint32_t handle);

	void ImportAssets();
	void ImportEngineAssets();
	void ImportAssets(const std::string& importDirectory, std::unordered_map<std::string, LazyAsset>& storage, AssetNameRegistry& registry, uint32_t& handle, bool isEngine = false);

	/**
	 * The key is the asset name with the subfolder. For example:
	 * Texture are usually in the folder Import/Textures so if you need a texture you would write Textures/your asset name
	 */
	std::unordered_map<std::string, LazyAsset> assets;
	AssetNameRegistry nameRegistry;

	std::unordered_map<std::string, LazyAsset> engineAssets;
	AssetNameRegistry engineNameRegistry;

	friend class Engine;
};


template <typename... Keys>
inline void AssetManager::QueryAssets(uint32_t assetHandles[], Keys... keys)
{
	uint32_t index = 0;
	(([&]
		{
			const auto it = this->assets.find(keys);
			if (it != this->assets.end())
			{
				const uint32_t handle = nameRegistry.GetHandle(keys);
				assetHandles[index] = handle;
				AddAssetRef(handle);
			}
			++index; }()),
		...);
}

template<typename ...Keys>
inline void AssetManager::QueryEngineAssets(uint32_t assetHandles[], Keys ...keys)
{
	uint32_t index = 0;
	(([&]
		{
			const auto it = this->engineAssets.find(keys);
			if (it != this->engineAssets.end())
			{
				const uint32_t handle = engineNameRegistry.GetHandle(keys);
				assetHandles[index] = handle;
				AddEngineAssetRef(handle);
			}
			++index; }()),
		...);
}

template<typename T>
inline T* AssetManager::LoadAsset(uint32_t handle)
{
	if ((handle & ENGINE_ASSET_FLAG) != 0)
	{
		auto it = engineAssets.find(engineNameRegistry.GetName(handle));
		if (it != engineAssets.end())
		{
			return it->second.Get<T>();
		}
	}
	else
	{
		auto it = assets.find(nameRegistry.GetName(handle));
		if (it != assets.end())
		{
			return it->second.Get<T>();
		}
	}

	return nullptr;
}
