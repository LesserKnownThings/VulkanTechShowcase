#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

struct aiNode;
struct aiScene;
struct AnimationData;
struct BoneNode;
struct SkeletonData;
struct MeshData;

class MeshImporter
{
public:
	static void ImportModel(const std::string& path, MeshData& outMeshData);
	static void ImportSkeleton(const std::string& path, SkeletonData& outSkeletonData);
	static bool ImportAnimation(const std::string& path, const SkeletonData& skeletonData, AnimationData& outAnimationData);

private:
	static void ProcessMeshForSkeleton(aiNode* node, const aiScene* scene, SkeletonData& skeletonData);
	static void ReadBoneHierarchyData(aiNode* src, SkeletonData& skeletonData, BoneNode& root);
};