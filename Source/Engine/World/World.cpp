#include "World.h"
#include "AssetManager/AssetManager.h"
#include "AssetManager/Model/MeshData.h"
#include "AssetManager/Model/Model.h"

#include "Camera/Camera.h"
#include "ECS/Components/Components.h"
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

	camera = new Camera(glm::vec3(0.0f, 25.0f, 100.0f));
	TaskManager::Get().RegisterTask(this, &World::Tick, TICK_HANDLE);
	TaskManager::Get().RegisterTask(this, &World::Draw, RENDER_HANDLE);

	GameEngine->GetInputSystem()->onMouseButtonPressedDelegate.Bind(this, &World::HandleMouseButton);
	RenderingInterface* renderingInterface = GameEngine->GetRenderingSystem();

	AssetManager& assetManager = AssetManager::Get();

	uint32_t handles[4];
	AssetManager::Get().QueryAssets(handles, "Meshes\\SM_Chr_PlagueDoctor_01", "Textures\\PolygonDarkFantasy_Texture_01_B", "Meshes\\Plane", "Textures\\Floor");

	// Floor setup
	entt::entity floor = registry.create();
	registry.emplace<Transform>(floor, Transform
		{
			.eulers = glm::vec3(-90.0f, 0.0f, 0.0f),
			.scale = glm::vec3(100.0f)			
		});
	registry.emplace<ModelComponent>(floor, ModelComponent{ handles[2] });

	std::vector<MaterialDescriptorBindingResource> floorProviders = { MaterialDescriptorBindingResource{MaterialSemantics::Albedo, handles[3]} };

	Model* floorModel = assetManager.LoadAsset<Model>(handles[2]);
	const MeshData& floorMeshData = floorModel->GetMeshData();

	renderingInterface->GetMaterialSystem()->SetTextures(floorMeshData.materials[0].materialInstanceHandle, floorProviders);
	// ************

	//entt::entity entity = registry.create();

	//// TODO this is a bit messy, maybe find a better way to set materials?
	//std::vector<MaterialDescriptorBindingResource> providers = { MaterialDescriptorBindingResource{MaterialSemantics::Albedo, handles[1]} };

	//Model* model = assetManager.LoadAsset<Model>(handles[0]);
	//const MeshData& meshData = model->GetMeshData();

	//renderingInterface->GetMaterialSystem()->SetTextures(meshData.materials[0].materialInstanceHandle, providers);
	//// *******************************

	//registry.emplace<Transform>(entity, Transform{ .scale = glm::vec3(0.25f) });
	//registry.emplace<ModelComponent>(entity, ModelComponent{ handles[0] });


	/*entt::entity directionalLight = registry.create();
	registry.emplace<Transform>(directionalLight, Transform{ .eulers = glm::vec3(35.0f, 15.0f, 0.0f) });
	CommonLightData dirLightData
	{
		Color::white,
		2.0f
	};
	Light dirLight = lightSystem->CreateLight(directionalLight, &dirLightData, ELightType::Directional);
	registry.emplace<Light>(directionalLight, dirLight);*/

	entt::entity lightEntity = registry.create();
	registry.emplace<Transform>(lightEntity,
		Transform
		{
			.position = glm::vec3(0.0f, 2.0f, 0.0f),
			.eulers = glm::vec3(90.0f, 0.0f, 0.0f)
		});
	SpotLightData lightData
	{
		Color::white,
		1.0f,
		19.5f,
		30.5f
	};
	Light light = lightSystem->CreateLight(lightEntity, &lightData, ELightType::Spot);
	registry.emplace<Light>(lightEntity, light);


	//entt::entity directionalLightEntity = registry.create();
	//Light directionalLight = lightSystem->CreateDirectionalLight(glm::vec3(0.0f));
	//registry.emplace<Light>(directionalLightEntity, directionalLight);
}

void World::UnInitialize()
{
	delete lightSystem;

	TaskManager::Get().RemoveAllTasks(this);

	auto view = registry.view<const ModelComponent>();
	// TODO move the meshes in a mesh system?
	view.each([](entt::entity entity, const ModelComponent& model)
		{
			AssetManager::Get().ReleaseAsset(model.handle);
		});

	delete camera;
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
		camera,
		registry,
		entities,
		lights
	};

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
}

void World::HandleCameraMovement(float deltaTime)
{
	InputSystem* inputSystem = GameEngine->GetInputSystem();

	const float horizontalAxis = inputSystem->GetHorizontalAxis();
	const float verticalAxis = inputSystem->GetVerticalAxis();

	const glm::vec3 forward = camera->GetForwardVector() * verticalAxis;
	const glm::vec3 right = camera->GetRightVector() * horizontalAxis;
	const glm::vec3 movement = forward + right;

	constexpr float CAM_MOVEMENT_SPEED = 30.0f;
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

		constexpr float CAM_LOOK_SPEED = 100.0f;

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
