#include "RenderingInterface.h"
#include "Engine.h"
#include "Input/InputSystem.h"
#include "TaskManager.h"

#include <SDL3/SDL.h>

bool RenderingInterface::Initialize(int32_t inWidth, int32_t inHeight)
{
	if (inWidth != 0 && inHeight != 0)
	{
		width = inWidth;
		height = inHeight;
	}

	InputSystem* inputSystem = GameEngine->GetInputSystem();
	inputSystem->onWindowResize.Bind(this, &RenderingInterface::HandleWindowResized);
	inputSystem->onWindowMinimized.Bind(this, &RenderingInterface::HandleWindowMinimized);

	return true;
}

void RenderingInterface::UnInitialize()
{
	InputSystem* inputSystem = GameEngine->GetInputSystem();
	inputSystem->onWindowResize.Clear(this);
	inputSystem->onWindowMinimized.Clear(this);
	SDL_DestroyWindow(window);
}

void RenderingInterface::HandleWindowResized()
{
	SDL_GetWindowSize(window, &width, &height);
	aspectRatio = static_cast<float>(width) / static_cast<float>(height);
	onWindowResizeParams.Invoke(static_cast<float>(width), static_cast<float>(height));
}

void RenderingInterface::RenderTasks()
{
	TaskManager::Get().ExecuteTasks(RENDER_HANDLE);
}
