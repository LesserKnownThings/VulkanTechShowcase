#pragma once

#include <string>
#include <vector>

struct MeshData;

class MeshImporter
{
public:
	/// <summary>
	/// This imports a model not a single mesh!!
	/// This means that if the model has multiple meshes they will be
	/// combined in a single vertex buffer and the index offset will
	/// be used to render the meshes
	/// </summary>
	/// <param name="path"></param>
	/// <param name="outModel"></param>
	static void ImportModel(const std::string& path, MeshData& outMeshData);
};