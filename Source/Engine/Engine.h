#pragma once

#include <cstdint>
#include <vector>

#define GameEngine Engine::Get()

class InputSystem;
class RenderingInterface;
class SDLInterface;
class SystemBase;
class World;

class Engine
{
public:
	static Engine* Get();

	Engine();

	bool Initialize();
	void UnInitialize();

	void Run();

	RenderingInterface* GetRenderingSystem() const { return renderingInterface; }

private:
	void InitializeECSSystems();
	void UnInitializeECSSystems();

	void HandleExit();

	InputSystem* inputSystem = nullptr;
	RenderingInterface* renderingInterface = nullptr;
	SDLInterface* sdlInterface = nullptr;
	
	World* currentWorld = nullptr;
	
	std::vector<SystemBase*> ecsSystems;

	bool isRunning = false;
	float deltaTime = 0.0f;

	static Engine* instance;
};