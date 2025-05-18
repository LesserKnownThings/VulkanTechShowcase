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

void AssetManager::ManualImportAsset(const std::string& assetName, const std::string& assetPath)
{
	assets.emplace(assetName, LazyAssetPtr{ AssetPath{assetPath} });
}

void AssetManager::ImportAssets()
{
	const std::string importDirectory = "Data/Import";

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
			assets.emplace(assetPath, LazyAssetPtr{ path });
		}
	}
}
