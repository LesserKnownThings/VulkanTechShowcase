#pragma once

#include <cstdint>
#include <vector>

#define GameEngine Engine::Get()

class InputSystem;
class RenderingInterface;
class SDLInterface;
class World;

class Engine
{
public:
	static Engine* Get();

	Engine();

	bool Initialize();
	void UnInitialize();

	void Run();

	float GetDeltaTime() const { return deltaTime; }

	RenderingInterface* GetRenderingSystem() const { return renderingInterface; }
	InputSystem* GetInputSystem() const { return inputSystem; }

private:
	void InitializeECSSystems();
	void UnInitializeECSSystems();

	void HandleExit();

	InputSystem* inputSystem = nullptr;
	RenderingInterface* renderingInterface = nullptr;
	SDLInterface* sdlInterface = nullptr;

	World* currentWorld = nullptr;

	bool isRunning = false;
	float deltaTime = 0.0f;

	static Engine* instance;
};