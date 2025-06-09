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
#include "Rendering/Descriptors/Semantics.h"
#include "Rendering/Light/Light.h"
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
	mainCamData.position = glm::vec3(0.0f, 25.0f, -100.0f);
	mainCam = cameraSystem->CreateCamera(mainCamData);

	TaskManager::Get().RegisterTask(this, &World::Tick, TICK_HANDLE);

	GameEngine->GetInputSystem()->onMouseButtonPressedDelegate.Bind(this, &World::HandleMouseButton);
	RenderingInterface* renderingInterface = GameEngine->GetRenderingSystem();

	entt::entity directionalLight = registry.create();
	registry.emplace<Transform>(directionalLight, Transform{ .eulers = glm::vec3(45.0f, 15.0f, 0.0f) });
	CommonLightData dirLightData
	{
		Color::white,
		2.0f
	};
	Light dirLight = lightSystem->CreateLight(directionalLight, &dirLightData, ELightType::Directional);
	registry.emplace<Light>(directionalLight, dirLight);

	CreateFloor();
	CreateCharacter(glm::vec3(0.0f, 0.0f, -50.0f));
	CreateCharacter(glm::vec3(0.0f));

	/*entt::entity pointEntity = registry.create();
	registry.emplace<Transform>(pointEntity, Transform{.position = glm::vec3(0.0f, 5.0f, -15.0f)});
	PointLightData pointData
	{
		Color::yellow,
		5.0f,
		100.0f
	};
	Light pointLight = lightSystem->CreateLight(pointEntity, &pointData, ELightType::Point);
	registry.emplace<Light>(pointEntity, pointLight);*/
}

void World::CreateCharacter(const glm::vec3& pos)
{
	AssetManager& assetManager = AssetManager::Get();
	RenderingInterface* renderingSystem = GameEngine->GetRenderingSystem();

	uint32_t handles[3];
	assetManager.QueryAssets(handles, "Animations\\Skeleton", "Textures\\PolygonDarkFantasy_Texture_01_B", "Animations\\Pointing");

	Model* model = assetManager.LoadAsset<Model>(handles[0]);
	const MeshData& mesh = model->GetMeshData();

	AnimatorComponent anim = animationSystem->CreateAnimator(handles[2], { "Data\\Import\\Animations\\Pointing.fbx" });

	MaterialDescriptorBindingResource resource{};
	resource.semantic = Semantics::AlbedoSampler;
	resource.textureAssetHandle = handles[1];
	renderingSystem->GetMaterialSystem()->SetTextures(mesh.materials[0].materialInstanceHandle, { resource });

	entt::entity hero = registry.create();
	registry.emplace<Transform>(hero, Transform{ .position = pos, .scale = glm::vec3(0.5f) });
	registry.emplace<ModelComponent>(hero, handles[0]);
	registry.emplace<AnimatorComponent>(hero, anim);
}

void World::CreateFloor()
{
	AssetManager& assetManager = AssetManager::Get();
	RenderingInterface* renderingSystem = GameEngine->GetRenderingSystem();

	uint32_t handles[2];
	assetManager.QueryAssets(handles, "Meshes\\Plane", "Textures\\Floor");

	Model* model = assetManager.LoadAsset<Model>(handles[0]);
	const MeshData& mesh = model->GetMeshData();

	MaterialDescriptorBindingResource resource{};
	resource.semantic = Semantics::AlbedoSampler;
	resource.textureAssetHandle = handles[1];
	renderingSystem->GetMaterialSystem()->SetTextures(mesh.materials[0].materialInstanceHandle, { resource });

	entt::entity hero = registry.create();
	registry.emplace<Transform>(hero, Transform{
		.eulers = glm::vec3(90, 0.0f, 0.0f),
		.scale = glm::vec3(500.0f) });
	registry.emplace<ModelComponent>(hero, handles[0]);
}

void World::UnInitialize()
{
	delete animationSystem;
	delete lightSystem;
	delete cameraSystem;

	TaskManager::Get().RemoveAllTasks(this);

	auto view = registry.view<const ModelComponent>();
	// TODO move the meshes in a mesh system?
	view.each([](entt::entity entity, const ModelComponent& model)
		{
			AssetManager::Get().ReleaseAsset(model.handle);
		});
}

void World::GetWorldView(View* view)
{
	auto entitiesInView = registry.view<const Transform, const ModelComponent>();

	entitiesInView.each([&](const auto entity, const Transform& transform, const ModelComponent& model)
		{
			view->entitiesInView.push_back(entity);
		});

	auto lightsInView = registry.view<const Light>();
	lightsInView.each([&](const auto entity, const Light& light)
		{
			view->lightsInView.push_back(entity);
		});

	view->registry = &registry;
	view->camera = &mainCam;
	view->animationSystem = animationSystem;
	view->lightSystem = lightSystem;
}

void World::Tick(float deltaTime)
{
	HandleCameraMovement(deltaTime);
	HandleCameraLook(deltaTime);

	auto tr = registry.view<Transform, const ModelComponent>();
	tr.each([&](entt::entity, Transform& transform, const ModelComponent& model)
		{
			//transform.eulers.y += 10.0f * deltaTime;			
		});

	auto view = registry.view<const Light>();
	view.each([&](entt::entity, const Light& light)
		{
			lightSystem->RotateLight(light.lightInstanceHandle, glm::vec3(1.0, 1.0f, 0.0f), 5.0f * deltaTime);
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

	const glm::vec3 forward = mainCam.data.forward * verticalAxis;
	const glm::vec3 right = mainCam.data.right * horizontalAxis;
	const glm::vec3 movement = forward + right;

	constexpr float CAM_MOVEMENT_SPEED = 30.0f;
	if (glm::length(movement) > 0.0f)
	{
		glm::vec3 pos = mainCam.data.position;
		pos += movement * CAM_MOVEMENT_SPEED * deltaTime;
		mainCam.SetPosition(pos);
	}
}

void World::HandleCameraLook(float deltaTime)
{
	if (isLooking)
	{
		InputSystem* inputSystem = GameEngine->GetInputSystem();

		constexpr float CAM_LOOK_SPEED = 150.0f;

		const glm::vec2& mouseAxis = inputSystem->GetMouseAxis();
		if (glm::length(mouseAxis) > 0.0f)
		{
			mainCam.Rotate(glm::vec3(mouseAxis.y, mouseAxis.x, 0.0f) * CAM_LOOK_SPEED * deltaTime);
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
