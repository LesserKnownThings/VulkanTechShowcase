#include "Skeleton.h"
#include "Utilities/MeshImporter.h"

bool Skeleton::LoadAsset(const std::string& path)
{
	MeshImporter::ImportSkeleton(path, skeletonData);
	return true;
}

void Skeleton::UnloadAsset()
{
}
