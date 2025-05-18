#pragma once

#include <string>
#include <vector>

struct MeshData;

class MeshImporter
{
public:
	static void ImportModel(const std::string& path, std::vector<MeshData>& outMeshes);
};