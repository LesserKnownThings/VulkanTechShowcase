#pragma once

#include "Utilities/Delegate.h"

#include <cstdint>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

enum ECameraType : uint8_t
{
    Orthographic,
    Perspective
};

constexpr glm::vec3 worldRight = glm::vec3(1.0f, 0.0f, 0.0f);
constexpr glm::vec3 worldUp = glm::vec3(0.0f, -1.0f, 0.0f);
constexpr glm::vec3 worldForward = glm::vec3(0.0f, 0.0f, -1.0f);

class Camera
{
public:
    ~Camera();
    Camera(const glm::vec3 &initialPosition = glm::vec3(0.0f, 0.0f, 10.0f), const glm::vec3 &initialRotation = glm::vec3(0.0f));

    void ChangeType(ECameraType inType);
    void SetFieldOfView(float inFieldOfView);
    void SetNearView(float inNearView);
    void SetFarView(float inFarView);    

    void SetOrthographicSize(float inOrthographicSize);
    void SetCameraZ(float inCameraZ);

    void SetPosition(const glm::vec3 &newPosition);
    void SetRotation(const glm::vec3 &eulerAngles);
    void Rotate(const glm::vec3 &axis, const glm::vec2& pitchClamp = glm::vec2(-89.0f, 89.0f));

    const glm::vec3 &GetPosition() const { return position; }
    const float &GetOrthographicSize() const { return ortographicSize; }
    const glm::vec3 GetEulers() const;
    const glm::vec3 &GetForwardVector() const;
    const glm::vec3 &GetUpVector() const;
    const glm::vec3 &GetRightVector() const;
    const glm::mat4 &GetView() const { return view; }
    const glm::mat4 &GetProjection() const { return projection; }
    const float GetNear() const { return nearView; }
    const float GetFar() const { return farView; }

    void UpdateProjection(std::function<void(const glm::mat4& projection)> func);
    void UpdateView(std::function<void(const glm::mat4& view)> func);

    const bool ProjectionChanged() const { return projectionChanged; }
    const bool ViewChanged() const { return viewChanged; }

protected:
    void UpdateVectors();

    void SetPerspectiveCamera();
    void SetOrthographicCamera();
    
    void UpdateProjectionType();
    void UpdateDirections();

    void HandleWindowResize(float width, float height);

    ECameraType cameraType = Perspective;

    //***Perspective settings
    float fieldOfView = 60.0f;
    float nearView = 1.0f;
    float farView = 1024.1f;

    //***Orthographic settings
    float ortographicSize = 10.0f;
    float cameraZ = 1000.0f;

    glm::vec3 position = glm::vec3(0.0f);

    glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);

    float pitch = 0.0f;
    float yaw = 0.0f;
    float roll = 0.0f;

    glm::quat rotation;

    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);

    bool projectionChanged = false;
    bool viewChanged = false;
};
