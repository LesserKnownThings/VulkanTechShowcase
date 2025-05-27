#include "World.h"
#include "AssetManager/AssetManager.h"
#include "Camera/Camera.h"
#include "Engine.h"
#include "Input/InputSystem.h"
#include "Rendering/AbstractData.h"
#include "Rendering/Light/Light.h"
#include "Rendering/RenderingInterface.h"
#include "TaskManager.h"
#include "ECS/Components/Components.h"
#include "ECS/Systems/LightSystem.h"
#include "ECS/Systems/MaterialSystem.h"

#include <SDL3/SDL.h>

void World::Initialize()
{
	lightSystem = new LightSystem();

	camera = new Camera(glm::vec3(0.0f, 60.0f, 500.0f));
	TaskManager::Get().RegisterTask(this, &World::Tick, TICK_HANDLE);
	TaskManager::Get().RegisterTask(this, &World::Draw, RENDER_HANDLE);

	GameEngine->GetInputSystem()->onMouseButtonPressedDelegate.Bind(this, &World::HandleMouseButton);

	entt::entity entity = registry.create();

	uint32_t handles[2];
	AssetManager::Get().QueryAssets(handles, "Meshes\\Demon", "Textures\\Polygon2");

	RenderingInterface* renderingInterface = GameEngine->GetRenderingSystem();
	Material material = renderingInterface->GetMaterialSystem()->CreatePBRMaterial(handles[1]);

	registry.emplace<Transform>(entity, Transform{});
	registry.emplace<Mesh>(entity, Mesh{ handles[0] });
	registry.emplace<Material>(entity, material);

	entt::entity lightEntity = registry.create();
	Light light = lightSystem->CreateDirectionalLight(glm::vec3(0.0f, 0.0f, 0.0f));
	registry.emplace<Light>(lightEntity, light);
}

void World::UnInitialize()
{
	delete lightSystem;

	TaskManager::Get().RemoveAllTasks(this);

	auto view = registry.view<const Mesh>();
	// TODO move the meshes in a mesh system?
	view.each([](entt::entity entity, const Mesh& mesh)
		{
			AssetManager::Get().ReleaseAsset(mesh.handle);
		});

	delete camera;
}

void World::Draw()
{
	RenderingInterface* renderingInterface = GameEngine->GetRenderingSystem();
	renderingInterface->UpdateView(camera->GetView());

	auto view = registry.view<const Transform, const Mesh, const Material>();
	std::vector<entt::entity> entities;
	view.each([&](const auto entity, const Transform& transform, const Mesh& mesh, const Material& material)
		{
			entities.push_back(entity);
		});

	GameEngine->GetRenderingSystem()->DrawSingle(entities, registry);
}

void World::Tick(float deltaTime)
{
	HandleCameraMovement(deltaTime);
	HandleCameraLook(deltaTime);

	auto view = registry.view<const Light>();
	view.each([&](entt::entity, const Light& light)
		{
			lightSystem->RotateLight(light.lightInstanceHandle, glm::vec3(0.0, 1.0f, 0.0f), 50.0f * deltaTime);
		});
}

void World::HandleCameraMovement(float deltaTime)
{
	InputSystem* inputSystem = GameEngine->GetInputSystem();

	const float horizontalAxis = inputSystem->GetHorizontalAxis();
	const float verticalAxis = inputSystem->GetVerticalAxis();

	const glm::vec3 forward = camera->GetForwardVector() * verticalAxis;
	const glm::vec3 right = camera->GetRightVector() * horizontalAxis;
	const glm::vec3 movement = forward + right;

	constexpr float CAM_MOVEMENT_SPEED = 160.0f;
	if (glm::length(movement) > 0.0f)
	{
		glm::vec3 pos = camera->GetPosition();
		pos += movement * CAM_MOVEMENT_SPEED * deltaTime;
		camera->SetPosition(pos);
	}
}

void World::HandleCameraLook(float deltaTime)
{
	if (isLooking)
	{
		InputSystem* inputSystem = GameEngine->GetInputSystem();

		constexpr float CAM_LOOK_SPEED = 140.0f;

		const glm::vec2& mouseAxis = inputSystem->GetMouseAxis();
		if (glm::length(mouseAxis) > 0.0f)
		{
			camera->Rotate(glm::vec3(-mouseAxis.y, -mouseAxis.x, 0.0f) * CAM_LOOK_SPEED * deltaTime);
		}
	}
}

void World::HandleMouseButton(uint8_t button, bool isPressed, const glm::vec2& position)
{
	if (button == 3)
	{
		isLooking = isPressed;

		InputSystem* inputSystem = GameEngine->GetInputSystem();
		inputSystem->SetMouseRelativeState(isPressed);
	}
}
