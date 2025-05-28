#include "MeshImporter.h"
#include "AssetManager/Model/MeshData.h"
#include "Engine.h"
#include "ECS/Systems/MaterialSystem.h"
#include "Rendering/RenderingInterface.h"

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

	void ProcessMesh(aiMesh* assimpMesh, const std::string& path, MeshData& outMeshData, size_t vertexOffset, size_t& globalIndexOffset)
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
	}

	void ProcessNodeForModel(aiNode* node, const aiScene* scene, const std::string& path, MeshData& meshData, size_t& globalVertexOffset, size_t& globalIndexOffset)
	{
		for (size_t i = 0; i < node->mNumMeshes; ++i)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshData.verticesCount += mesh->mNumVertices;
			meshData.offset.push_back(globalIndexOffset);
			meshData.meshesCount++;

			ProcessMesh(mesh, path, meshData, globalVertexOffset, globalIndexOffset);

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
			ProcessNodeForModel(node->mChildren[i], scene, path, meshData, globalVertexOffset, globalIndexOffset);
		}
	}

	void ProcessNodeForModel(aiNode* node, const aiScene* scene, const std::string& path, MeshData& meshData)
	{
		size_t globalVertexOffset = 0;
		size_t globalIndexOffset = 0;

		ProcessNodeForModel(node, scene, path, meshData, globalVertexOffset, globalIndexOffset);
	}
}

void MeshImporter::ImportModel(const std::string& path, MeshData& outMeshData)
{
	Assimp::Importer import;
	const aiScene * scene = import.ReadFile(path,
		aiProcess_Triangulate
		| aiProcess_GenSmoothNormals
		| aiProcess_FlipUVs
		| aiProcess_CalcTangentSpace
		| aiProcess_JoinIdenticalVertices);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
		return;
	}

	Utilities::ProcessNodeForModel(scene->mRootNode, scene, path, outMeshData);
}
