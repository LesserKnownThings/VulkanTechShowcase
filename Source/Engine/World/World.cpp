#include "World.h"
#include "AssetManager/AssetManager.h"
#include "AssetManager/Model/MeshData.h"
#include "AssetManager/Model/Model.h"
#include "AssetManager/Animation/Skeleton.h"
#include "Camera/Camera.h"

#include "ECS/Systems/AnimationSystem.h"
#include "ECS/Components/Components.h"
#include "ECS/Systems/CameraSystem.h"
#include "ECS/Systems/LightSystem.h"
#include "ECS/Systems/MaterialSystem.h"
#include "Engine.h"
#include "Input/InputSystem.h"
#include "Rendering/AbstractData.h"
#include "Rendering/Descriptors/DescriptorInfo.h"
#include "Rendering/Light/Light.h"
#include "Rendering/Material/MaterialSemantics.h"
#include "Rendering/RenderingInterface.h"
#include "TaskManager.h"
#include "View.h"

#include <SDL3/SDL.h>

void World::Initialize()
{
	lightSystem = new LightSystem(registry);
	animationSystem = new AnimationSystem();
	cameraSystem = new CameraSystem();

	CameraData mainCamData{};
	mainCamData.position = glm::vec3(0.0f, 25.0f, 100.0f);
	mainCam = cameraSystem->CreateCamera(mainCamData);

	TaskManager::Get().RegisterTask(this, &World::Tick, TICK_HANDLE);
	TaskManager::Get().RegisterTask(this, &World::Draw, RENDER_HANDLE);

	GameEngine->GetInputSystem()->onMouseButtonPressedDelegate.Bind(this, &World::HandleMouseButton);
	RenderingInterface* renderingInterface = GameEngine->GetRenderingSystem();

	entt::entity directionalLight = registry.create();
	registry.emplace<Transform>(directionalLight, Transform{ .eulers = glm::vec3(35.0f, 15.0f, 0.0f) });
	CommonLightData dirLightData
	{
		Color::white,
		2.0f
	};
	Light dirLight = lightSystem->CreateLight(directionalLight, &dirLightData, ELightType::Directional);
	registry.emplace<Light>(directionalLight, dirLight);
}

void World::UnInitialize()
{
	delete animationSystem;
	delete lightSystem;

	TaskManager::Get().RemoveAllTasks(this);

	auto view = registry.view<const ModelComponent>();
	// TODO move the meshes in a mesh system?
	view.each([](entt::entity entity, const ModelComponent& model)
		{
			AssetManager::Get().ReleaseAsset(model.handle);
		});
}

void World::Draw()
{
	RenderingInterface* renderingInterface = GameEngine->GetRenderingSystem();

	auto enttView = registry.view<const Transform, const ModelComponent>();
	std::vector<entt::entity> entities;
	enttView.each([&](const auto entity, const Transform& transform, const ModelComponent& model)
		{
			entities.push_back(entity);
		});

	auto lightsView = registry.view<const Light>();
	std::vector<entt::entity> lights;
	lightsView.each([&](const auto entity, const Light& light)
		{
			lights.push_back(entity);
		});

	View view
	{
		*mainCam,
		registry,
		entities,
		lights,

		animationSystem,
		lightSystem
	};


	auto animators = registry.view<const AnimatorComponent>();
	animators.each([&](entt::entity, const AnimatorComponent& component)
		{
			//animationSystem->UpdateDrawData(component.animatorInstanceHandle);
		});

	GameEngine->GetRenderingSystem()->DrawSingle(view);
}

void World::Tick(float deltaTime)
{
	HandleCameraMovement(deltaTime);
	HandleCameraLook(deltaTime);

	auto view = registry.view<const Light>();
	view.each([&](entt::entity, const Light& light)
		{
			//lightSystem->RotateLight(light.lightInstanceHandle, glm::vec3(1.0, 1.0f, 0.0f), 50.0f * deltaTime);
		});

	auto animators = registry.view<const AnimatorComponent>();
	animators.each([&](entt::entity, const AnimatorComponent& component)
		{
			animationSystem->Run(component.animatorInstanceHandle, deltaTime);
		});
}

void World::HandleCameraMovement(float deltaTime)
{
	InputSystem* inputSystem = GameEngine->GetInputSystem();

	const float horizontalAxis = inputSystem->GetHorizontalAxis();
	const float verticalAxis = inputSystem->GetVerticalAxis();

	const glm::vec3 forward = mainCam->data.forward * verticalAxis;
	const glm::vec3 right = mainCam->data.right * horizontalAxis;
	const glm::vec3 movement = forward + right;

	constexpr float CAM_MOVEMENT_SPEED = 30.0f;
	if (glm::length(movement) > 0.0f)
	{
		glm::vec3 pos = mainCam->data.position;
		pos += movement * CAM_MOVEMENT_SPEED * deltaTime;
		mainCam->SetPosition(pos);
	}
}

void World::HandleCameraLook(float deltaTime)
{
	if (isLooking)
	{
		InputSystem* inputSystem = GameEngine->GetInputSystem();

		constexpr float CAM_LOOK_SPEED = 2.45f;

		const glm::vec2& mouseAxis = inputSystem->GetMouseAxis();
		if (glm::length(mouseAxis) > 0.0f)
		{
			mainCam->Rotate(glm::vec3(-mouseAxis.y, -mouseAxis.x, 0.0f) * CAM_LOOK_SPEED * deltaTime);
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
