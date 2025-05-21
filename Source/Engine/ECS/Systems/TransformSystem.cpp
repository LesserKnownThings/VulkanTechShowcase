#include "TransformSystem.h"
#include "ECS/Entity.h"

#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

TransformSystem& TransformSystem::Get()
{
	static TransformSystem instance;
	return instance;
}

void TransformSystem::UnInitialize()
{
	free(component.buffer);
	instances.clear();
}

uint32_t TransformSystem::CreateComponent(const Entity& entity, uint8_t isStatic, const glm::vec3& position, const glm::vec3& eulers, const glm::vec3& scale)
{
	const uint32_t cID = component.instances;

	if (component.allocatedInstances <= cID)
	{
		Allocate(component.allocatedInstances * 2);
	}

	component.instances++;

	instances[entity.id] = cID;

	component.entity[cID] = entity;
	component.model[cID] = glm::mat4(1.0f);
	component.position[cID] = position;
	component.eulers[cID] = eulers;
	component.scale[cID] = scale;
	component.isStatic[cID] = isStatic;
	component.state[cID] = EComponentState::Ready;

	RecalculateModel(cID);
	return cID;
}

void TransformSystem::HandleEntityRemoved(const Entity& entity)
{
	auto it = instances.find(entity.id);
	if (it != instances.end())
	{
		component.state[it->second] = EComponentState::Dead;
		pendingDeletion.push_back(it->second);
	}
}

void TransformSystem::DestroyComponent(uint32_t componentID)
{
	const uint32_t last = component.instances - 1;
	const Entity currentEntity = component.entity[componentID];
	const Entity lastEntity = component.entity[last];

	component.entity[componentID] = component.entity[last];
	component.model[componentID] = component.model[last];
	component.position[componentID] = component.position[last];
	component.eulers[componentID] = component.eulers[last];
	component.scale[componentID] = component.scale[last];
	component.isStatic[componentID] = component.isStatic[last];
	component.state[componentID] = component.state[last];

	instances[lastEntity.id] = componentID;
	instances.erase(currentEntity.id);
	component.instances--;
}

bool TransformSystem::TryGetPosition(const Entity& entity, glm::vec3& outPosition) const
{
	const auto it = instances.find(entity);
	if (it != instances.end())
	{
		outPosition = component.position[it->second];
		return true;
	}
	return false;
}

bool TransformSystem::TryGetScale(const Entity& entity, glm::vec3& scale) const
{
	const auto it = instances.find(entity);
	if (it != instances.end())
	{
		scale = component.scale[it->second];
		return true;
	}
	return false;
}

bool TransformSystem::TryGetModel(const Entity& entity, glm::mat4& outModel) const
{
	const auto it = instances.find(entity);
	if (it != instances.end())
	{
		outModel = component.model[it->second];
		return true;
	}
	return false;
}

void TransformSystem::SetPosition(const Entity& entity, const glm::vec3& position)
{
	const uint32_t cID = instances[entity];
	component.position[cID] = position;
	RecalculateModel(cID);
}

void TransformSystem::Rotate(const Entity& entity, float angle, const glm::vec3& axis)
{
	const uint32_t cID = instances[entity];
	glm::vec3 eulers = component.eulers[cID];
	eulers += glm::vec3(angle * axis.x, angle * axis.y, angle * axis.z);
	eulers = glm::mod(eulers, 360.0f);
	component.eulers[cID] = eulers;
	RecalculateModel(cID);
}

void TransformSystem::SetRotation(const Entity& entity, const glm::vec3& eulers)
{
	const uint32_t cID = instances[entity];
	component.eulers[cID] = eulers;
}

void TransformSystem::SetSize(const Entity& entity, const glm::vec3& scale)
{
	auto it = instances.find(entity);
	if (it != instances.end())
	{
		component.scale[it->second] = scale;
		RecalculateModel(it->second);
	}
}

void TransformSystem::RecalculateModel(uint32_t componentID)
{
	glm::mat4& model = component.model[componentID];

	const glm::vec3& position = component.position[componentID];
	const glm::vec3& eulers = component.eulers[componentID];
	const glm::vec3& scale = component.scale[componentID];

	model = glm::translate(glm::mat4(1.0f), position);
	model = glm::rotate(model, glm::radians(eulers.x), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(eulers.y), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(eulers.z), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, scale);
}

void TransformSystem::Allocate(int32_t size)
{
	TransformComponent newComp;

	newComp.instances = component.instances;
	newComp.allocatedInstances = size;

	newComp.buffer = malloc(size * (sizeof(Entity) + sizeof(glm::mat4) + sizeof(glm::vec3) * 3 + sizeof(uint8_t) + sizeof(EComponentState)));

	newComp.model = (glm::mat4*)(newComp.buffer);
	newComp.position = (glm::vec3*)(size + newComp.model);
	newComp.eulers = size + newComp.position;
	newComp.scale = size + newComp.eulers;
	newComp.entity = (Entity*)(size + newComp.scale);
	newComp.state = (EComponentState*)(size + newComp.entity);	newComp.isStatic = (uint8_t*)(size + newComp.state);


	memcpy(newComp.model, component.model, component.instances * sizeof(glm::mat4));
	memcpy(newComp.position, component.position, component.instances * sizeof(glm::vec3));
	memcpy(newComp.eulers, component.eulers, component.instances * sizeof(glm::vec3));
	memcpy(newComp.scale, component.scale, component.instances * sizeof(glm::vec3));
	memcpy(newComp.entity, component.entity, component.instances * sizeof(Entity));
	memcpy(newComp.state, component.state, component.instances * sizeof(EComponentState));
	memcpy(newComp.isStatic, component.isStatic, component.instances * sizeof(uint8_t));	

	free(component.buffer);
	component = newComp;
}
