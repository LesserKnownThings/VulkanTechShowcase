#include "World.h"
#include "Camera/Camera.h"
#include "Engine.h"
#include "Rendering/RenderingInterface.h"
#include "TaskManager.h"

#include "AssetManager/AssetManager.h"
#include "AssetManager/LazyAssetPtr.h"
#include "AssetManager/Model/Model.h"
#include "ECS/EntityManager.h"
#include "ECS/Systems/MeshSystem.h"
#include "ECS/Systems/TransformSystem.h"

void World::Initialize()
{
	camera = new Camera();
	TaskManager::Get().RegisterTask(this, &World::Tick, TICK_HANDLE);
	TaskManager::Get().RegisterTask(this, &World::Draw, RENDER_HANDLE);

	Entity entity = EntityManager::Get().CreateEntity();

	AssetManager::Get().GetAsset<Model>("Meshes\\Cube", [entity](LazyAssetPtr& asset)
		{
			MeshSystem::Get().CreateComponent(entity, asset);
		});
	TransformSystem::Get().CreateComponent(entity, true);
	entities.push_back(entity);

}

void World::UnInitialize()
{
	TaskManager::Get().RemoveAllTasks(this);
	delete camera;
}

void World::Draw()
{
	RenderingInterface* renderingInterface = GameEngine->GetRenderingSystem();
	renderingInterface->UpdateView(camera->GetView());

	for (const Entity& entity : entities)
	{
		renderingInterface->DrawSingle(entity);
	}
}

void World::Tick(float deltaTime)
{
	TransformSystem::Get().Rotate(entities[0], 5.0f * deltaTime * 15.0f, glm::vec3(1.0f));
}
