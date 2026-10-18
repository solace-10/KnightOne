#pragma once

#include <array>
#include <vector>

#include <glm/vec3.hpp>

#include <input/input_system.hpp>
#include <scene/systems/system.hpp>

namespace WingsOfSteel
{

class Camera;

class CameraSystem : public System
{
public:
    CameraSystem();
    ~CameraSystem();

    void Initialize(Scene* pScene) override;
    void Update(float delta) override;

    // Convert mouse screen coordinates to world space coordinates on the XZ plane (Y = 0)
    // mousePos: screen coordinates (0,0 at top-left, width/height at bottom-right)
    // Returns: world space position on the XZ plane, or (0,0,0) if no active camera or no intersection
    glm::vec3 MouseToWorld(const glm::vec2& mousePos) const;

    // Debug camera: WASD to fly, hold the right mouse button to look around.
    bool IsFreeCameraEnabled() const { return m_FreeCameraEnabled; }
    void SetFreeCameraEnabled(bool enabled);
    float GetFreeCameraSpeed() const { return m_FreeCameraSpeed; }
    void SetFreeCameraSpeed(float speed) { m_FreeCameraSpeed = speed; }

private:
    void UpdateFreeCamera(float delta, Camera& camera);

    enum FreeCameraKey
    {
        Forward,
        Left,
        Back,
        Right,
        Fast,

        Count
    };

    InputCallbackToken m_RightMouseButtonPressedToken{ InputSystem::sInvalidInputCallbackToken };
    InputCallbackToken m_RightMouseButtonReleasedToken{ InputSystem::sInvalidInputCallbackToken };
    InputCallbackToken m_MousePositionToken{ InputSystem::sInvalidInputCallbackToken };
    InputCallbackToken m_MouseWheelToken{ InputSystem::sInvalidInputCallbackToken };
    bool m_IsDragging = false;
    bool m_InputPending = true;
    glm::vec2 m_MouseDelta{ 0.0f, 0.0f };

    std::vector<InputCallbackToken> m_KeyboardTokens;
    std::array<bool, FreeCameraKey::Count> m_FreeCameraKeysHeld{};
    bool m_FreeCameraEnabled = false;
    float m_FreeCameraSpeed = 50.0f;
    glm::vec3 m_FreeCameraPosition{ 0.0f };
    float m_FreeCameraYaw = 0.0f;
    float m_FreeCameraPitch = 0.0f;
};

} // namespace WingsOfSteel