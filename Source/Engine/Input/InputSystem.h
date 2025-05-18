#pragma once

#include "Utilities/Delegate.h"

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

enum class EKeyPressType : uint8_t
{
    Press,
    Release,
    Hold
};

DECLARE_DELEGATE(OnWindowResize);
DECLARE_DELEGATE(OnCloseApp);
DECLARE_DELEGATE_TwoParams(OnKeyPressed, uint32_t, EKeyPressType);
DECLARE_DELEGATE_ThreeParams(OnMouseButtonPressed, uint8_t, bool, const glm::vec2&);
DECLARE_DELEGATE_OneParam(OnMouseMove, const glm::vec2&);
DECLARE_DELEGATE_OneParam(OnTextInput, const char*);

class InputSystem
{
public:
    ~InputSystem();

    void RunEvents();

    float GetHorizontalAxis() const { return horizontalAxis; }
    float GetVerticalAxis() const { return verticalAxis; }
    const glm::vec2 &GetMouseAxis() const { return mouseAxis; }
    const glm::vec2 &GetMousePosition() const { return mousePosition; }

    void SetMouseRelativeState(bool isRelativeState);

    OnKeyPressed onKeyPressDelegate;
    OnMouseButtonPressed onMouseButtonPressedDelegate;
    OnMouseMove onMouseMoveDelegate;
    OnCloseApp onCloseAppDelegate;
    OnTextInput onTextInputDelegate;

    OnWindowResize onWindowResize;
    OnWindowResize onWindowMinimized;

private:
    InputSystem();    
    void ProcessMousePressInput(float deltaTime);

    void HandlePress(uint32_t &ctrlMask);
    void HandleRelease(uint32_t &ctrlMask);
    void HandleHold(uint32_t &ctrlMask);

    void HandleAxisInput();

    glm::vec2 lastKnownMousePosition = glm::vec2(0.0f);

    float horizontalAxis = 0.0f;
    float verticalAxis = 0.0f;

    glm::vec2 mousePosition = glm::vec2(0.0f);
    glm::vec2 mouseAxis = glm::vec2(0.0f);

    std::vector<uint32_t> pressedKeys;
    std::vector<uint32_t> releasedKeys;
    std::vector<uint32_t> heldKeys;
    uint32_t cachedMouseButton = 0;

    friend class Engine;
};