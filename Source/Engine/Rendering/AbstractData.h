#pragma once

#include <cstdint>
#include <variant>

using GenericHandle = std::variant<uint32_t, uintptr_t>;

/// <summary>
/// I'm abstracting this for both open GL and Vulkan
/// One of the memory handles can be used for the vao on open GL
/// </summary>
struct MeshRenderData
{
	GenericHandle vertexBuffer;
	GenericHandle indexBuffer;

	GenericHandle vertexMemory;
	GenericHandle indexMemory;
};