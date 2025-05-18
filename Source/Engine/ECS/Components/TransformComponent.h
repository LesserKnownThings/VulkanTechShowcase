#pragma once

#include "Component.h"
#include <glm/glm.hpp>

struct TransformComponent : public Component
{
    glm::mat4 *model = nullptr;
    glm::vec3 *position = nullptr; 
    glm::vec3 *eulers = nullptr;
    glm::vec3 *scale = nullptr;
    uint8_t *isStatic = nullptr;
};