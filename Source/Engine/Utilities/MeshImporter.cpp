#include "MeshImporter.h"
#include "AssetManager/Animation/AnimationData.h"
#include "AssetManager/Animation/BoneData.h"
#include "AssetManager/Model/MeshData.h"
#include "Engine.h"
#include "EngineName.h"
#include "ECS/Systems/MaterialSystem.h"
#include "Rendering/RenderingInterface.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <limits>

namespace AssimpGLMHelpers
{
	static inline glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from)
	{
		glm::mat4 to;
		// the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
		to[0][0] = from.a1;
		to[1][0] = from.a2;
		to[2][0] = from.a3;
		to[3][0] = from.a4;
		to[0][1] = from.b1;
		to[1][1] = from.b2;
		to[2][1] = from.b3;
		to[3][1] = from.b4;
		to[0][2] = from.c1;
		to[1][2] = from.c2;
		to[2][2] = from.c3;
		to[3][2] = from.c4;
		to[0][3] = from.d1;
		to[1][3] = from.d2;
		to[2][3] = from.d3;
		to[3][3] = from.d4;
		return to;
	}

	static inline glm::vec3 GetGLMVec(const aiVector3D& vec)
	{
		return glm::vec3(vec.x, vec.y, vec.z);
	}

	static inline glm::quat GetGLMQuat(const aiQuaternion& pOrientation)
	{
		return glm::quat(pOrientation.w, pOrientation.x, pOrientation.y, pOrientation.z);
	}
}

namespace Utilities
{
	// TODO remove this? I don't I'll use it?
	// Since imported elements from Blender have a 2x2 space need to normalize them to 1x1
	void NormalizeVertices(MeshData& meshData)
	{
		glm::vec3 minBounds(std::numeric_limits<float>::max());
		glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

		for (int32_t i = 0; i < meshData.verticesCount; ++i)
		{
			minBounds = glm::min(minBounds, meshData.vertices[i].position);
			maxBounds = glm::max(maxBounds, meshData.vertices[i].position);
		}

		glm::vec3 size = maxBounds - minBounds;
		glm::vec3 invSize
		{
			size.x > 0.0f ? 1.0f / size.x : 0.0f,
			size.y > 0.0f ? 1.0f / size.y : 0.0f,
			size.z > 0.0f ? 1.0f / size.z : 0.0f
		};

		for (int32_t i = 0; i < meshData.verticesCount; ++i)
		{
			glm::vec3& v = meshData.vertices[i].position;
			v = (v - minBounds) * invSize;
		}
	}

	void ExtractSkeletonFromModel(aiMesh* mesh, SkeletonData& skeletonData)
	{
		for (int32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
		{
			std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();

			EngineName eBoneName{ boneName };

			auto it = skeletonData.boneInfoMap.find(eBoneName);
			if (it == skeletonData.boneInfoMap.end())
			{
				BoneInfo newBone{};
				newBone.id = skeletonData.boneInfoCount++;
				newBone.offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
				skeletonData.boneInfoMap.emplace(boneName, newBone);
			}
		}

		std::string test;
	}

	void MatchAnimationSkeletonBones(const aiAnimation* animation, const SkeletonData& skeletonData, AnimationData& animationData)
	{
		const int32_t size = animation->mNumChannels;

		const auto& boneInfoMap = skeletonData.boneInfoMap;
		const int32_t boneInfoCount = skeletonData.boneInfoCount;

		for (int32_t i = 0; i < size; ++i)
		{
			auto channel = animation->mChannels[i];
			EngineName boneName = EngineName{ channel->mNodeName.C_Str() };

			auto it = boneInfoMap.find(boneName);
			if (it == boneInfoMap.end())
			{
#if WITH_EDITOR
				std::cerr << "Missing bone: " << boneName.editorName << " in skeleton, will skip animation bone!" << std::endl;
#endif
				continue;
			}

			BoneInstance bone{};

			bone.numPositions = channel->mNumPositionKeys;
			for (int32_t j = 0; j < bone.numPositions; ++j)
			{
				bone.positions.emplace_back(
					KeyPosition
					{
						AssimpGLMHelpers::GetGLMVec(channel->mPositionKeys[j].mValue),
						static_cast<float>(channel->mPositionKeys[j].mTime)
					}
				);
			}

			bone.numRotations = channel->mNumRotationKeys;
			for (int32_t j = 0; j < bone.numRotations; ++j)
			{
				bone.rotations.emplace_back(
					KeyRotation
					{
						AssimpGLMHelpers::GetGLMQuat(channel->mRotationKeys[j].mValue),
						static_cast<float>(channel->mPositionKeys[j].mTime)
					}
				);
			}

			bone.numScales = channel->mNumScalingKeys;
			for (int32_t j = 0; j < bone.numScales; ++j)
			{
				bone.scales.emplace_back(
					KeyScale
					{
						AssimpGLMHelpers::GetGLMVec(channel->mScalingKeys[j].mValue),
						static_cast<float>(channel->mScalingKeys[j].mTime)
					}
				);
			}

			animationData.boneInstanceMap[boneName] = bone;
		}
	}

	void ProcessMesh(aiMesh* assimpMesh, MeshData& outMeshData, size_t vertexOffset, size_t& globalIndexOffset)
	{
		for (size_t i = 0; i < assimpMesh->mNumVertices; i++)
		{
			glm::vec3 position{};
			position.x = assimpMesh->mVertices[i].x;
			position.y = assimpMesh->mVertices[i].y;
			position.z = assimpMesh->mVertices[i].z;

			glm::vec3 tangent{};
			tangent.x = assimpMesh->mTangents[i].x;
			tangent.y = assimpMesh->mTangents[i].y;
			tangent.z = assimpMesh->mTangents[i].z;

			glm::vec3 bitangent{};
			bitangent.x = assimpMesh->mBitangents[i].x;
			bitangent.y = assimpMesh->mBitangents[i].y;
			bitangent.z = assimpMesh->mBitangents[i].z;

			glm::vec3 normal{};
			if (assimpMesh->HasNormals())
			{
				normal.x = assimpMesh->mNormals[i].x;
				normal.y = assimpMesh->mNormals[i].y;
				normal.z = assimpMesh->mNormals[i].z;
			}

			glm::vec2 uv{};
			if (assimpMesh->mTextureCoords[0])
			{
				// TODO will need to test this with different models
				uv.x = assimpMesh->mTextureCoords[0][i].x;
				uv.y = assimpMesh->mTextureCoords[0][i].y;
			}
			else
				uv = glm::vec2(0.0f, 0.0f);

			outMeshData.vertices.emplace_back(Vertex{ position, normal, tangent, bitangent, uv });
		}

		MeshIndexData meshIndexData{};

		for (unsigned int i = 0; i < assimpMesh->mNumFaces; i++)
		{
			aiFace face = assimpMesh->mFaces[i];

			for (unsigned int j = 0; j < face.mNumIndices; j++)
			{
				meshIndexData.indices.push_back(face.mIndices[j] + static_cast<uint32_t>(vertexOffset));
				meshIndexData.count++;
			}
		}

		outMeshData.meshIndices.push_back(meshIndexData);
		outMeshData.indicesCount += meshIndexData.count;
		globalIndexOffset += meshIndexData.count;


		std::unordered_map<EngineName, uint32_t> bonesMap;
		uint32_t boneCount = 0;
		for (int32_t boneIndex = 0; boneIndex < assimpMesh->mNumBones; ++boneIndex)
		{
			int32_t boneID = -1;
			std::string boneName = assimpMesh->mBones[boneIndex]->mName.C_Str();

			EngineName eBoneName{ boneName };

			auto it = bonesMap.find(eBoneName);
			if (it == bonesMap.end())
			{
				boneID = boneCount++;
				bonesMap.emplace(boneName, boneID);
			}
			else
			{
				boneID = it->second;
			}

			aiVertexWeight* weights = assimpMesh->mBones[boneIndex]->mWeights;
			int32_t numWeights = assimpMesh->mBones[boneIndex]->mNumWeights;

			for (int32_t weightIndex = 0; weightIndex < numWeights; ++weightIndex)
			{
				int32_t vertexId = weights[weightIndex].mVertexId;
				float weight = weights[weightIndex].mWeight;

				for (int32_t i = 0; i < MAX_BONE_INFLUENCE; ++i)
				{
					if (outMeshData.vertices[vertexId].boneIDs[i] < 0)
					{
						outMeshData.vertices[vertexId].boneIDs[i] = boneID;
						outMeshData.vertices[vertexId].weights[i] = weight;
						break;
					}
				}
			}
		}

		std::string test;
	}

	void ProcessNodeForModel(aiNode* node, const aiScene* scene, MeshData& meshData, size_t& globalVertexOffset, size_t& globalIndexOffset)
	{
		for (size_t i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshData.verticesCount += mesh->mNumVertices;
			meshData.offset.push_back(globalIndexOffset);
			meshData.meshesCount++;

			ProcessMesh(mesh, meshData, globalVertexOffset, globalIndexOffset);

			/*
			* Supporting materials from the mesh is very frustrating especially when a lot of people don't
			* properly set the materials. So each mesh will have the base engine textures.
			*/
			if (scene->HasMaterials())
			{
				for (int32_t i = 0; i < scene->mNumMaterials; ++i)
				{
					meshData.materials.push_back(GameEngine->GetRenderingSystem()->GetMaterialSystem()->CreatePBRMaterial(std::nullopt, true));
				}
			}

			globalVertexOffset += meshData.verticesCount;
		}

		for (size_t i = 0; i < node->mNumChildren; ++i)
		{
			ProcessNodeForModel(node->mChildren[i], scene, meshData, globalVertexOffset, globalIndexOffset);
		}
	}

	void ProcessNodeForModel(aiNode* node, const aiScene* scene, MeshData& meshData)
	{
		size_t globalVertexOffset = 0;
		size_t globalIndexOffset = 0;

		ProcessNodeForModel(node, scene, meshData, globalVertexOffset, globalIndexOffset);
	}
}

void MeshImporter::ImportModel(const std::string& path, MeshData& outMeshData)
{
	Assimp::Importer import;
	const aiScene * scene = import.ReadFile(path,
		aiProcess_Triangulate
		| aiProcess_GenSmoothNormals
		| aiProcess_FlipUVs
		| aiProcess_CalcTangentSpace);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
		return;
	}

	Utilities::ProcessNodeForModel(scene->mRootNode, scene, outMeshData);
}

void MeshImporter::ImportSkeleton(const std::string& path, SkeletonData& outSkeletonData)
{
	Assimp::Importer import;
	const aiScene * scene = import.ReadFile(path,
		aiProcess_Triangulate);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
		return;
	}

	ProcessMeshForSkeleton(scene->mRootNode, scene, outSkeletonData);

	std::vector<glm::mat4> previousTransforms{};
	ReadBoneHierarchyData(scene->mRootNode, outSkeletonData, outSkeletonData.rootBone, previousTransforms);
}

bool MeshImporter::ImportAnimation(const std::string& path, const SkeletonData& skeletonData, AnimationData& outAnimationData)
{
	Assimp::Importer import;
	const aiScene * scene = import.ReadFile(path,
		aiProcess_Triangulate);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
		return false;
	}

	// Only supporting 1 animtion per import
	if (scene->mNumAnimations > 0)
	{
		auto animation = scene->mAnimations[0];
		outAnimationData.duration = animation->mDuration;
		outAnimationData.ticksPerSecond = animation->mTicksPerSecond;
		Utilities::MatchAnimationSkeletonBones(animation, skeletonData, outAnimationData);
		return true;
	}

	return false;
}

void MeshImporter::ProcessMeshForSkeleton(aiNode* node, const aiScene* scene, SkeletonData& skeletonData)
{
	for (size_t i = 0; i < node->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		Utilities::ExtractSkeletonFromModel(mesh, skeletonData);

		// This will return when the first mesh is encountered, that's because the skeleton only supports 1 mesh per model
		return;
	}

	for (size_t i = 0; i < node->mNumChildren; ++i)
	{
		ProcessMeshForSkeleton(node->mChildren[i], scene, skeletonData);
	}
}

void MeshImporter::ReadTempHierarchyData(aiNode* src, const SkeletonData& skeletonData, BoneNode& root, std::vector<glm::mat4>& previousTrasforms)
{
	previousTrasforms.emplace_back(AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation));

	for (int i = 0; i < src->mNumChildren; ++i)
	{
		std::string name = src->mChildren[i]->mName.C_Str();
		if (name.find("Assimp") != std::string::npos)
		{
			ReadTempHierarchyData(src->mChildren[i], skeletonData, root, previousTrasforms);
		}
		else
		{
			BoneNode newData{};
			ReadBoneHierarchyData(src->mChildren[i], skeletonData, newData, previousTrasforms);
			root.children.push_back(newData);
		}
	}
}

void MeshImporter::ReadBoneHierarchyData(aiNode* src, const SkeletonData& skeletonData, BoneNode& root, std::vector<glm::mat4>& previousTrasforms)
{
	root.name = EngineName{ src->mName.C_Str() };
	root.transform = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);	
	for (const glm::mat4& transform : previousTrasforms)
	{
		root.transform *= transform;
	}
	previousTrasforms.clear();
	root.childrenCount = src->mNumChildren;

	for (int i = 0; i < src->mNumChildren; ++i)
	{
		std::string name = src->mChildren[i]->mName.C_Str();
		if (name.find("Assimp") != std::string::npos)
		{
			ReadTempHierarchyData(src->mChildren[i], skeletonData, root, previousTrasforms);
		}
		else
		{
			BoneNode newData{};
			ReadBoneHierarchyData(src->mChildren[i], skeletonData, newData, previousTrasforms);
			root.children.push_back(newData);
		}
	}
}
