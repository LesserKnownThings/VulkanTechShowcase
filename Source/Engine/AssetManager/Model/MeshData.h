#pragma once

#include <cstdint>
#include <glm/glm.hpp>

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec3 biTangent;
	glm::vec2 uv;
};

struct MeshData
{
	int32_t verticesCount = 0;
	int32_t indicesCount = 0;

	std::vector<uint32_t> indices;
	std::vector<Vertex> vertices;
};