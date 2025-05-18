#pragma once

#include <filesystem>
#include <string>

struct AssetPath
{
    AssetPath() = default;
    AssetPath(const std::filesystem::path &inFullPath)
        : fullPath(inFullPath.string()), assetName(inFullPath.stem().string()), extension(inFullPath.extension().string())
    {
    }

    std::string fullPath;
    std::string assetName;
    std::string extension;
};

struct AssetPathHash
{
    size_t operator()(const AssetPath &assetPath) const
    {
        std::hash<std::string> h;
        return h(assetPath.fullPath);
    }
};
