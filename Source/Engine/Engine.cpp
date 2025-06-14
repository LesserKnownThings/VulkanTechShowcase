#include "Engine.h"
#include "AssetManager/AssetManager.h"
#include "Input/InputSystem.h"
#include "Rendering/Vulkan/VulkanRendering.h"
#include "SDLInterface.h"
#include "TaskManager.h"
#include "World/World.h"

#include <SDL3/SDL.h>

// How much MS for each FPS
constexpr uint32_t FPS_30 = 33;
constexpr uint32_t FPS_60 = 16;
constexpr uint32_t FPS_120 = 8;

Engine* Engine::instance = nullptr;
Engine* Engine::Get()
{
	return instance;
}

Engine::Engine()
{
	instance = this;
}

bool Engine::Initialize()
{
	renderingInterface = new VulkanRendering();
	sdlInterface = new SDLInterface();
	inputSystem = new InputSystem();

	inputSystem->onCloseAppDelegate.Bind(this, &Engine::HandleExit);

	AssetManager& assetManager = AssetManager::Get();
	assetManager.ImportAssets();
	assetManager.ImportEngineAssets();

	InitializeECSSystems();

	const bool success = sdlInterface->Initialize() &&
		renderingInterface->Initialize(960, 540);

	if (success)
	{
		currentWorld = new World();
		currentWorld->Initialize();
	}

	return success;
}

void Engine::UnInitialize()
{
	UnInitializeECSSystems();

	currentWorld->UnInitialize();

	renderingInterface->UnInitialize();
	delete renderingInterface;
	renderingInterface = nullptr;

	sdlInterface->UnInitialize();
	delete sdlInterface;
	sdlInterface = nullptr;

	AssetManager::Get().UnInitialize();
}

void Engine::Run()
{
	isRunning = true;

	constexpr float MAX_DELTA = .016f;
	constexpr float GC_PASS_DELAY = 2.5f;

	uint64_t currentTicks = SDL_GetTicks();

	float currentGCDelay = 0.0f;

	while (isRunning)
	{
		uint64_t newTicks = SDL_GetTicks();

		deltaTime = (newTicks - currentTicks) / 1000.0f;

		if (deltaTime > MAX_DELTA)
			deltaTime = MAX_DELTA;

		currentTicks = newTicks;

		const uint64_t frameStart = SDL_GetTicks();

		// Input
		inputSystem->RunEvents();

		//Tick
		TaskManager::Get().ExecuteTasks(TICK_HANDLE, deltaTime);

		// Rendering
		renderingInterface->DrawFrame();
		renderingInterface->EndFrame();

		const uint64_t frameDuration = SDL_GetTicks() - frameStart;

		// TODO maybe move this to a different thread?
		// GC
		currentGCDelay += deltaTime;
		if (currentGCDelay >= GC_PASS_DELAY)
		{
			currentGCDelay = 0.0f;
			TaskManager::Get().ExecuteTasks(GC_HANDLE);
		}

		// TODO move this to an arg or settings
		if (FPS_120 > frameDuration)
		{
			SDL_Delay(FPS_120);
		}
	}
}

void Engine::InitializeECSSystems()
{

}

void Engine::UnInitializeECSSystems()
{

}

void Engine::HandleExit()
{
	isRunning = false;
}
