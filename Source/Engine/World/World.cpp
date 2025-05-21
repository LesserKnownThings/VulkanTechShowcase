#include "World.h"
#include "Camera/Camera.h"
#include "Engine.h"
#include "Rendering/AbstractData.h"
#include "Rendering/RenderingInterface.h"
#include "TaskManager.h"

#include "AssetManager/AssetManager.h"
#include "AssetManager/LazyAssetPtr.h"
#include "AssetManager/Model/Model.h"
#include "AssetManager/Texture/Texture.h"
#include "ECS/EntityManager.h"
#include "ECS/Systems/MaterialSystem.h"
#include "ECS/Systems/MeshSystem.h"
#include "ECS/Systems/TransformSystem.h"

void World::Initialize()
{
	camera = new Camera(glm::vec3(0.0f, 1.5f, 3.5f));
	TaskManager::Get().RegisterTask(this, &World::Tick, TICK_HANDLE);
	TaskManager::Get().RegisterTask(this, &World::Draw, RENDER_HANDLE);

	Entity entity = EntityManager::Get().CreateEntity();

	LazyAssetPtr assets[2];
	AssetManager::Get().QueryAssets<Texture, Model>(assets, "Textures\\viking_room", "Meshes\\viking_room");

	if (assets[0].IsValid() && assets[1].IsValid())
	{
		MaterialSystem::Get().CreateComponent(entity, static_cast<uint8_t>(EPipelineType::PBR), assets, 1);
		MeshSystem::Get().CreateComponent(entity, assets[1]);
	}

	TransformSystem::Get().CreateComponent(entity, true, glm::vec3(0.0f), glm::vec3(-90.0f, 0.0f, -135.0f), glm::vec3(2.0f));
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
	//TransformSystem::Get().Rotate(entities[0], 100.0f * deltaTime, glm::vec3(0.0f, 0.0f, -1.0f));
}
