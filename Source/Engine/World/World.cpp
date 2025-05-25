#include "World.h"
#include "AssetManager/AssetManager.h"
#include "Camera/Camera.h"
#include "Engine.h"
#include "Rendering/AbstractData.h"
#include "Rendering/RenderingInterface.h"
#include "TaskManager.h"
#include "ECS/Components/Components.h"
#include "ECS/Systems/MaterialSystem.h"


void World::Initialize()
{
	camera = new Camera(glm::vec3(0.0f, 60.0f, 500.0f));
	TaskManager::Get().RegisterTask(this, &World::Tick, TICK_HANDLE);
	TaskManager::Get().RegisterTask(this, &World::Draw, RENDER_HANDLE);

	entt::entity entity = registry.create();

	uint32_t handles[2];
	AssetManager::Get().QueryAssets(handles, "Meshes\\Demon", "Textures\\Polygon2");

	Material material = GameEngine->GetMaterialSystem()->CreatePBRMaterial(handles[1]);

	registry.emplace<Transform>(entity, Transform{});
	registry.emplace<Mesh>(entity, Mesh{ handles[0] });
	registry.emplace<Material>(entity, material);
}

void World::UnInitialize()
{
	TaskManager::Get().RemoveAllTasks(this);

	auto view = registry.view<Mesh, Material>();
	// TODO release resources when world is destroyed
	view.each([](entt::entity entity, const Mesh& mesh, const Material& material)
		{

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

}
