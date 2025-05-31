#pragma once

#include <string>
#include <vector>

struct AnimationData;
struct SkeletonData;
struct MeshData;

class MeshImporter
{
public:
	static void ImportModel(const std::string& path, MeshData& outMeshData);
	static void ImportSkeleton(const std::string& path, SkeletonData& outSkeletonData);
	static bool ImportAnimation(const std::string& path, const SkeletonData& skeletonData, AnimationData& outAnimationData);
};