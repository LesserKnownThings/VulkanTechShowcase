#include "MeshImporter.h"
#include "AssetManager/Model/MeshData.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <iostream>
#include <limits>

namespace Utilities
{
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

	void ProcessMesh(aiMesh* assimpMesh, const std::string& path, MeshData& outMeshData)
	{
		for (size_t i = 0; i < assimpMesh->mNumVertices; i++)
		{
			outMeshData.vertices[i].position.x = assimpMesh->mVertices[i].x;
			outMeshData.vertices[i].position.y = assimpMesh->mVertices[i].y;
			outMeshData.vertices[i].position.z = assimpMesh->mVertices[i].z;

			outMeshData.vertices[i].tangent.x = assimpMesh->mTangents[i].x;
			outMeshData.vertices[i].tangent.y = assimpMesh->mTangents[i].y;
			outMeshData.vertices[i].tangent.z = assimpMesh->mTangents[i].z;

			outMeshData.vertices[i].biTangent.x = assimpMesh->mBitangents[i].x;
			outMeshData.vertices[i].biTangent.y = assimpMesh->mBitangents[i].y;
			outMeshData.vertices[i].biTangent.z = assimpMesh->mBitangents[i].z;

			if (assimpMesh->HasNormals())
			{
				outMeshData.vertices[i].normal.x = assimpMesh->mNormals[i].x;
				outMeshData.vertices[i].normal.y = assimpMesh->mNormals[i].y;
				outMeshData.vertices[i].normal.z = assimpMesh->mNormals[i].z;
			}			

			if (assimpMesh->mTextureCoords[0])
			{
				// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
				// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
				outMeshData.vertices[i].uv.x = assimpMesh->mTextureCoords[0][i].x;
				outMeshData.vertices[i].uv.y = assimpMesh->mTextureCoords[0][i].y;
			}
			else
				outMeshData.vertices[i].uv = glm::vec2(0.0f, 0.0f);
		}

		//NormalizeVertices(outMeshData);

		for (unsigned int i = 0; i < assimpMesh->mNumFaces; i++)
		{
			aiFace face = assimpMesh->mFaces[i];

			for (unsigned int j = 0; j < face.mNumIndices; j++)
			{
				outMeshData.indices.push_back(face.mIndices[j]);
				outMeshData.indicesCount++;
			}
		}
	}

	void ProcessNodeForModel(aiNode* node, const aiScene* scene, const std::string& path, std::vector<MeshData>& outMeshes)
	{
		for (size_t i = 0; i < node->mNumMeshes; ++i)
		{
			MeshData meshData;
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshData.verticesCount = mesh->mNumVertices;
			meshData.vertices.resize(mesh->mNumVertices);
			ProcessMesh(mesh, path, meshData);
			outMeshes.push_back(meshData);
		}

		for (size_t i = 0; i < node->mNumChildren; ++i)
		{
			ProcessNodeForModel(node->mChildren[i], scene, path, outMeshes);
		}
	}
}

void MeshImporter::ImportModel(const std::string& path, std::vector<MeshData>& outMeshes)
{
	Assimp::Importer import;
	const aiScene * scene = import.ReadFile(path,
		aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
		return;
	}

	Utilities::ProcessNodeForModel(scene->mRootNode, scene, path, outMeshes);
}
