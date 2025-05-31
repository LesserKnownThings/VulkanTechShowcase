#include "InputSystem.h"
#include "Engine.h"
#include "Rendering/RenderingInterface.h"
#include "TaskManager.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>

#if WITH_EDITOR
#include <imgui/imgui_impl_sdl3.h>
#endif

InputSystem::~InputSystem()
{
}

InputSystem::InputSystem()
{
	TaskManager::Get().RegisterTask(this, &InputSystem::ProcessMousePressInput, TICK_HANDLE);
}

void InputSystem::SetMouseRelativeState(bool isRelativeState)
{
	SDL_Window* window = GameEngine->GetRenderingSystem()->GetWindow();

	if (isRelativeState)
	{
		SDL_GetMouseState(&lastKnownMousePosition.x, &lastKnownMousePosition.y);
	}
	else
	{
		SDL_WarpMouseInWindow(window, lastKnownMousePosition.x, lastKnownMousePosition.y);
	}

	SDL_SetWindowRelativeMouseMode(window, isRelativeState);
}

void InputSystem::RunEvents()
{
	float deltaX, deltaY;
	SDL_GetRelativeMouseState(&deltaX, &deltaY);

	constexpr float MIN_ACCUMULATED_DELTA = 3.0f;

	mouseAxis = glm::vec2(0.0f);
	if (std::abs(deltaX) >= MIN_ACCUMULATED_DELTA)
	{
		mouseAxis.x = glm::clamp(deltaX, -1.0f, 1.0f);
	}

	if (std::abs(deltaY) >= MIN_ACCUMULATED_DELTA)
	{
		mouseAxis.y = glm::clamp(deltaY, -1.0f, 1.0f);
	}

	SDL_GetMouseState(&mousePosition.x, &mousePosition.y);

	static SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		if (e.type == SDL_EVENT_QUIT)
		{
			onCloseAppDelegate.Invoke();
		}
		else if (e.type == SDL_EVENT_WINDOW_RESIZED)
		{
			onWindowResize.Invoke();
		}
		else if (e.type == SDL_EVENT_WINDOW_MINIMIZED)
		{
			onWindowMinimized.Invoke();
		}
		else
		{
			if (e.key.down)
			{
				if (e.key.repeat == 0)
				{
					pressedKeys.push_back(e.key.key);
				}

				heldKeys.push_back(e.key.key);
			}
			else if (!e.key.down)
			{
				if (e.key.repeat == 0)
				{
					releasedKeys.push_back(e.key.key);
				}
				heldKeys.clear();
			}

			glm::vec2 mousePos;
			const float windowHeight = static_cast<float>(GameEngine->GetRenderingSystem()->GetWindowHeight());
			SDL_GetMouseState(&mousePos.x, &mousePos.y);
			mousePos.y = windowHeight - mousePos.y;

			if (e.type == SDL_EVENT_MOUSE_MOTION)
			{
				onMouseMoveDelegate.Invoke(mousePos);
			}

			if (e.button.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
			{
				if (!(cachedMouseButton & e.button.button))
				{
					cachedMouseButton |= e.button.button;
					onMouseButtonPressedDelegate.Invoke(e.button.button, true, mousePos);
				}
			}
			else if (e.button.type == SDL_EVENT_MOUSE_BUTTON_UP)
			{
				onMouseButtonPressedDelegate.Invoke(e.button.button, false, mousePos);
				cachedMouseButton &= ~e.button.button;
			}
		}

#if WITH_EDITOR
		ImGui_ImplSDL3_ProcessEvent(&e);
#endif
	}
}

void InputSystem::ProcessMousePressInput(float deltaTime)
{
	static uint32_t ctrlMask;

	HandlePress(ctrlMask);
	HandleRelease(ctrlMask);
	HandleHold(ctrlMask);

	pressedKeys.clear();
	releasedKeys.clear();
}

void InputSystem::HandlePress(uint32_t& ctrlMask)
{
	for (int32_t i = 0; i < pressedKeys.size(); ++i)
	{
		onKeyPressDelegate.Invoke(pressedKeys[i], EKeyPressType::Press);

		if (pressedKeys[i] == SDLK_W || pressedKeys[i] == SDLK_UP)
		{
			verticalAxis++;
		}
		else if (pressedKeys[i] == SDLK_S || pressedKeys[i] == SDLK_DOWN)
		{
			verticalAxis--;
		}

		if (pressedKeys[i] == SDLK_A || pressedKeys[i] == SDLK_LEFT)
		{
			horizontalAxis--;
		}
		else if (pressedKeys[i] == SDLK_D || pressedKeys[i] == SDLK_RIGHT)
		{
			horizontalAxis++;
		}

		if (pressedKeys[i] == SDLK_LCTRL)
		{
			ctrlMask |= 1;
		}

		if (ctrlMask & 1 && pressedKeys[i] == SDLK_1)
		{

		}
	}
}

void InputSystem::HandleRelease(uint32_t& ctrlMask)
{
	for (int32_t i = 0; i < releasedKeys.size(); ++i)
	{
		onKeyPressDelegate.Invoke(releasedKeys[i], EKeyPressType::Release);

		if (releasedKeys[i] == SDLK_W || releasedKeys[i] == SDLK_UP)
		{
			verticalAxis--;
		}
		else if (releasedKeys[i] == SDLK_S || releasedKeys[i] == SDLK_DOWN)
		{
			verticalAxis++;
		}

		if (releasedKeys[i] == SDLK_A || releasedKeys[i] == SDLK_LEFT)
		{
			horizontalAxis++;
		}
		else if (releasedKeys[i] == SDLK_D || releasedKeys[i] == SDLK_RIGHT)
		{
			horizontalAxis--;
		}

		if (releasedKeys[i] == SDLK_LCTRL)
		{
			ctrlMask &= ~1;
		}
	}
}

void InputSystem::HandleHold(uint32_t& ctrlMask)
{
	for (int32_t i = 0; i < heldKeys.size(); ++i)
	{
		onKeyPressDelegate.Invoke(heldKeys[i], EKeyPressType::Hold);
	}
}

void InputSystem::HandleAxisInput()
{
}
