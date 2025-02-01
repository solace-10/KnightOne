#include <render/debug_render.hpp>
#include <scene/components/camera_component.hpp>
#include <scene/components/orbit_camera_component.hpp>
#include <scene/components/transform_component.hpp>
#include <scene/entity.hpp>
#include <core/log.hpp>
#include <pandora.hpp>

#include "components/sub_sector_camera_component.hpp"
#include "components/ship_navigation_component.hpp"
#include "systems/camera_system.hpp"

namespace WingsOfSteel::TheBrightestStar
{

CameraSystem::CameraSystem()
{

}
    
CameraSystem::~CameraSystem()
{
    Pandora::InputSystem* pInputSystem = Pandora::GetInputSystem();
    if (pInputSystem)
    {
        pInputSystem->RemoveMouseButtonCallback(m_RightMouseButtonPressedToken);
        pInputSystem->RemoveMouseButtonCallback(m_RightMouseButtonReleasedToken);
        pInputSystem->RemoveMousePositionCallback(m_MousePositionToken);
    }
}

void CameraSystem::Initialize()
{
    using namespace Pandora;
    m_RightMouseButtonPressedToken = GetInputSystem()->AddMouseButtonCallback([this](){ m_IsDragging = true; }, MouseButton::Right, MouseAction::Pressed);
    m_RightMouseButtonReleasedToken = GetInputSystem()->AddMouseButtonCallback([this](){ m_IsDragging = false; }, MouseButton::Right, MouseAction::Released);

    m_MousePositionToken = GetInputSystem()->AddMousePositionCallback([this](const glm::vec2& mousePosition, const glm::vec2& mouseDelta){ 
        m_InputPending = true;
        m_MouseDelta = mouseDelta;
    });
}

void CameraSystem::Update(float delta)
{
    using namespace Pandora;
    EntitySharedPtr pCamera = GetActiveScene() ? GetActiveScene()->GetCamera(): nullptr;
    if (pCamera == nullptr)
    {
        return;
    }

    if (pCamera->HasComponent<CameraComponent>())
    {
        if (pCamera->HasComponent<SubSectorCameraComponent>())
        {
            SubSectorCameraComponent& subSectorCameraComponent = pCamera->GetComponent<SubSectorCameraComponent>();

            EntitySharedPtr pAnchorEntity = subSectorCameraComponent.anchorEntity.lock();
            glm::vec3 anchorPosition(0.0f);
            glm::vec3 cameraWantedTarget = anchorPosition;
            if (pAnchorEntity && pAnchorEntity->HasComponent<TransformComponent>())
            {
                const glm::mat4& anchorTransform = pAnchorEntity->GetComponent<TransformComponent>().transform;
                anchorPosition = glm::vec3(anchorTransform[3]);

                if (pAnchorEntity->HasComponent<ShipNavigationComponent>())
                {
                    const ShipNavigationComponent& shipNavigationComponent = pAnchorEntity->GetComponent<ShipNavigationComponent>();
                    glm::vec4 forward = anchorTransform[2];
                    const float forwardMultiplier = glm::clamp(shipNavigationComponent.GetThrust(), -1.0f, 1.0f);
                    cameraWantedTarget = anchorPosition + glm::vec3(forward) * forwardMultiplier * 30.0f;
                }
            }

            glm::vec3 cameraPosition = subSectorCameraComponent.position + anchorPosition;

            DampSpring(subSectorCameraComponent.target, cameraWantedTarget, subSectorCameraComponent.velocity, 1.0f, delta);

            CameraComponent& cameraComponent = pCamera->GetComponent<CameraComponent>();
            cameraComponent.camera.LookAt(cameraPosition, subSectorCameraComponent.target, glm::vec3(0.0f, 1.0f, 0.0f));

            if (subSectorCameraComponent.debugDraw)
            {
                GetDebugRender()->Circle(subSectorCameraComponent.target, glm::vec3(0.0f, 1.0f, 0.0f), Pandora::Color::Red, 2.0f, 10.0f);
                GetDebugRender()->Circle(cameraWantedTarget, glm::vec3(0.0f, 1.0f, 0.0f), Pandora::Color::Green, 2.0f, 10.0f);
            }
        }
        else if (pCamera->HasComponent<OrbitCameraComponent>())
        {
            OrbitCameraComponent& orbitCameraComponent = pCamera->GetComponent<OrbitCameraComponent>();
            if (m_IsDragging && m_InputPending)
            {
                const float sensitivity = 0.15f;
                orbitCameraComponent.orbitAngle -= glm::radians(m_MouseDelta.x * sensitivity);
                orbitCameraComponent.pitch += glm::radians(m_MouseDelta.y * sensitivity);

                if (orbitCameraComponent.pitch < orbitCameraComponent.minimumPitch)
                {
                    orbitCameraComponent.pitch = orbitCameraComponent.minimumPitch;
                }
                else if (orbitCameraComponent.pitch > orbitCameraComponent.maximumPitch)
                {
                    orbitCameraComponent.pitch = orbitCameraComponent.maximumPitch;
                }
                m_InputPending = false;
            }

            glm::vec3 position(
                glm::cos(orbitCameraComponent.orbitAngle) * glm::cos(orbitCameraComponent.pitch),
                glm::sin(orbitCameraComponent.pitch),
                glm::sin(orbitCameraComponent.orbitAngle) * glm::cos(orbitCameraComponent.pitch)
            );

            EntitySharedPtr pAnchorEntity = orbitCameraComponent.anchorEntity.lock();
            glm::vec3 anchorPosition(0.0f);
            if (pAnchorEntity && pAnchorEntity->HasComponent<TransformComponent>())
            {
                const glm::mat4& anchorTransform = pAnchorEntity->GetComponent<TransformComponent>().transform;
                anchorPosition = glm::vec3(anchorTransform[3]);
            }

            CameraComponent& cameraComponent = pCamera->GetComponent<CameraComponent>();
            cameraComponent.camera.LookAt(anchorPosition + position * orbitCameraComponent.distance, anchorPosition, glm::vec3(0.0f, 1.0f, 0.0f));
        }
    }
}

// Found in https://gamedev.net/forums/topic/329868-damped-spring-effects-for-camera/3149147/?page=1
// Use a damped spring to move v0 towards target given a current velocity, time over which the spring would 
// cover 90% of the distance from rest and the delta time.
void CameraSystem::DampSpring(glm::vec3& v0, const glm::vec3& target, glm::vec3& velocity, float time90, float delta) const
{
    const float c0 = delta * 3.75f / time90;	
    if (c0 >= 1.0f)	
    {		
        // If our distance to the target is too small, we go the whole way to prevent oscillation.		
        v0 = target;		
        velocity = glm::vec3(0.0f);		
        return;	
    }	
    
    const glm::vec3 diff = target - v0;	
    const glm::vec3 force = diff - 2.0f * velocity;	
    v0 += velocity * c0;
    velocity += force * c0;   
}

} // namespace WingsOfSteel::TheBrightestStar