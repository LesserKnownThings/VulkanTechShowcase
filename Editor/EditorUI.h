#pragma once

#include <cstdint>

class RenderingInterface;
class Window;

class EditorUI
{
public:
	bool Initialize(RenderingInterface* renderingSystem);
	void UnInitialize();

	void DrawFrame(uintptr_t commandBuffer);

private:
	Window* mainWindow = nullptr;
};