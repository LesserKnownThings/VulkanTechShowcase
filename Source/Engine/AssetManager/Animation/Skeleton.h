#pragma once

#include "AssetManager/Asset.h"
#include "BoneData.h"

class Skeleton : public Asset
{
public:
	virtual bool LoadAsset(const std::string& path) override;
	virtual void UnloadAsset() override;

	const SkeletonData& GetSkeletonData() const { return skeletonData; }

private:
	SkeletonData skeletonData;
};