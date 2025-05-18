#include "Engine.h"
#include "AssetManager/AssetManager.h"
#include "ECS/Systems/MeshSystem.h"
#include "ECS/Systems/TransformSystem.h"
#include "Input/InputSystem.h"
#include "Rendering/Vulkan/VulkanRendering.h"
#include "SDLInterface.h"
#include "TaskManager.h"
#include "World/World.h"

#include <SDL3/SDL.h>

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

	AssetManager::Get().ImportAssets();
	InitializeECSSystems();

	const bool success = sdlInterface->Initialize() &&
		renderingInterface->Initialize();

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

	constexpr int32_t FRAME_RATE = 16; // 60 fps
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

		if (FRAME_RATE > frameDuration)
		{
			SDL_Delay(FRAME_RATE);
		}
	}
}

void Engine::InitializeECSSystems()
{
	MeshSystem* meshSystem = &MeshSystem::Get();
	meshSystem->Initialize();
	ecsSystems.push_back(meshSystem);

	TransformSystem* transformSystem = &TransformSystem::Get();
	transformSystem->Initialize();
	ecsSystems.push_back(transformSystem);
}

void Engine::UnInitializeECSSystems()
{
	for (SystemBase* system : ecsSystems)
	{
		system->UnInitialize();
	}
	ecsSystems.clear();
}

void Engine::HandleExit()
{
	isRunning = false;
}
