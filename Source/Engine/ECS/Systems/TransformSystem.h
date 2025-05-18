#pragma once

#include "ECS/Components/TransformComponent.h"
#include "ECS/Entity.h"
#include "SystemBase.h"

#include <cstdint>
#include <unordered_map>

struct Entity;

class TransformSystem : public SystemBase
{
public:
    static TransformSystem &Get();

    void UnInitialize() override;

    uint32_t CreateComponent(const Entity &entity, uint8_t isStatic, const glm::vec3 &position = glm::vec3(0.0f), const glm::vec3 &eulers = glm::vec3(0.0f), const glm::vec3 &scale = glm::vec3(1.0f));

    bool TryGetPosition(const Entity& entity, glm::vec3 &outPosition) const;
    bool TryGetScale(const Entity& entity, glm::vec3& scale) const;
    bool TryGetModel(const Entity& entity, glm::mat4 &outModel) const;

    /**
     * The setter functions do not check if the entity exists
     * If the entity does not exist a component will be created for that entity, so be careful with the setters
     */
    void SetPosition(const Entity& entity, const glm::vec3 &position);
    void SetRotation(const Entity& entity, const glm::vec3 &eulers);
    void SetSize(const Entity& entity, const glm::vec3 &scale);
    void Rotate(const Entity& entity, float angle, const glm::vec3 &axis);

private:
    void RecalculateModel(uint32_t componentID);

    void Allocate(int32_t size) override;
    void DestroyComponent(uint32_t componentID) override;
    void HandleEntityRemoved(const Entity &entity) override;

    TransformComponent component;

    std::unordered_map<Entity, uint32_t> instances;
};