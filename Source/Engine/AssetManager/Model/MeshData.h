#pragma once

#include "AssetManager/Animation/BoneData.h"

#include <cstdint>
#include <glm/glm.hpp>

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 biTangent;
	glm::vec2 uv;
	int32_t boneIDs[MAX_BONE_INFLUENCE]{ -1, -1, -1, -1 };
	float weights[MAX_BONE_INFLUENCE];
};

struct MeshIndexData
{
	std::vector<uint32_t> indices;
	uint32_t count;
};

struct Material
{
	uint8_t pipeline;
	uint32_t materialInstanceHandle = 0;
};

struct MeshData
{
	int32_t verticesCount = 0;
	int32_t indicesCount = 0;
	uint32_t meshesCount = 0;

	std::vector<MeshIndexData> meshIndices;
	std::vector<size_t> offset;
	std::vector<Vertex> vertices;
	std::vector<Material> materials;
};