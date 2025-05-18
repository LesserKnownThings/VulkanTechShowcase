#include "RenderingInterface.h"
#include "TaskManager.h"

#include <SDL3/SDL.h>

bool RenderingInterface::Initialize(int32_t inWidth, int32_t inHeight)
{
	if (inWidth != 0 && inHeight != 0)
	{
		width = inWidth;
		height = inHeight;
	}

	return true;
}

void RenderingInterface::UnInitialize()
{
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
