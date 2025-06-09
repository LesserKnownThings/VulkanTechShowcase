#include "RenderingInterface.h"
#include "Descriptors/DescriptorRegistry.h"
#include "ECS/Systems/MaterialSystem.h"
#include "Engine.h"
#include "Input/InputSystem.h"

#include <SDL3/SDL.h>

uint32_t RenderingInterface::MIN_UNIFORM_ALIGNMENT = 64;

bool RenderingInterface::Initialize(int32_t inWidth, int32_t inHeight)
{
	if (inWidth != 0 && inHeight != 0)
	{
		width = inWidth;
		height = inHeight;
	}

	materialSystem = new MaterialSystem();

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

	delete descriptorRegistry;
	delete materialSystem;
	SDL_DestroyWindow(window);
}

void RenderingInterface::HandleWindowResized()
{
	SDL_GetWindowSize(window, &width, &height);
	aspectRatio = static_cast<float>(width) / static_cast<float>(height);
	onWindowResizeParams.Invoke(static_cast<float>(width), static_cast<float>(height));
}

void RenderingInterface::CreateDescriptorRegistry()
{
	descriptorRegistry = new DescriptorRegistry(this);
	descriptorRegistry->Initialize();
}
