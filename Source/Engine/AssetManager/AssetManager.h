#pragma once

#include "LazyAssetPtr.h"

#include <functional>
#include <string>
#include <unordered_map>

class AssetManager
{
public:
    static AssetManager& Get();

    void UnInitialize();

    template <typename T>
    void GetAsset(const std::string& assetPath, std::function<void(LazyAssetPtr& asset)> func);

    template <typename... Args, typename... Keys>
    void QueryAssets(LazyAssetPtr assets[], Keys... keys);


    void ManualImportAsset(const std::string& assetName, const std::string& assetPath);

private:
    AssetManager() = default;
    void ImportAssets();

    /**
     * The key is the asset name with the subfolder. For example:
     * Texture are usually in the folder Import/Textures so if you need a texture you would write Textures/your asset name
     */
    std::unordered_map<std::string, LazyAssetPtr> assets;

    friend class Engine;
};

template <typename T>
inline void AssetManager::GetAsset(const std::string& assetPath, std::function<void(LazyAssetPtr& asset)> func)
{
    auto it = assets.find(assetPath);
    if (it != assets.end())
    {
        func(it->second.StrongRef<T>());
    }
}

template <typename... Args, typename... Keys>
inline void AssetManager::QueryAssets(LazyAssetPtr outAssets[], Keys... keys)
{
    static_assert(sizeof...(Args) == sizeof...(Keys), "Number of types must match number of keys!");

    uint32_t index = 0;

    (([&]
        {
            auto it = this->assets.find(keys);
            if (it != this->assets.end()) {
                outAssets[index] = std::move(it->second.StrongRef<Args>());
            }
            ++index; }()),
        ...);
}
