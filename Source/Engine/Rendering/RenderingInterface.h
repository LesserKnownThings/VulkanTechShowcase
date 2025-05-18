#pragma once

#include "AbstractData.h"
#include "Utilities/Delegate.h"

#include <cstdint>
#include <glm/glm.hpp>
#include <SDL3/SDL_video.h>
#include <string>

struct EngineName;
struct Entity;
struct MeshData;
struct MeshRenderData;

DECLARE_DELEGATE_TwoParams(OnWindowResizeParams, float, float);

class RenderingInterface
{
public:
	virtual bool Initialize(int32_t inWidth = 0, int32_t inHeight = 0);
	virtual void UnInitialize();

	virtual void DrawFrame() = 0;
	virtual void EndFrame() = 0;

	virtual void DrawSingle(const Entity& entity) = 0;

	// Setters
	virtual void UpdateProjection(const glm::mat4& projection) = 0;
	virtual void UpdateView(const glm::mat4& view)  = 0;

	virtual void CreateMeshVertexBuffer(const MeshData& meshData, MeshRenderData& outRenderData) = 0;
	virtual void DestroyBuffer(GenericHandle buffer, GenericHandle bufferMemory) = 0;
	virtual void DestroyMeshVertexBuffer(const MeshRenderData& renderData) = 0;

	SDL_Window* GetWindow() const { return window; }
	int32_t GetWindowWidth() const { return width; }
	int32_t GetWindowHeight() const { return height; }
	float GetAspectRatio() const { return aspectRatio; }

	OnWindowResizeParams onWindowResizeParams;

protected:
	virtual void HandleWindowResized();
	virtual void HandleWindowMinimized() = 0;
	void RenderTasks();

	int32_t width = 960;
	int32_t height = 540;

	float aspectRatio = 1.0f;

	SDL_Window* window = nullptr;
	const std::string applicationName = "Tech Showcase";
};